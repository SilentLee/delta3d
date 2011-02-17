/*
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2008 MOVES Institute
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Author: Eric R. Heine
 */

#include <dtDirectorQt/nodescene.h>

#include <dtDirector/directorgraph.h>
#include <dtDirector/nodemanager.h>
#include <dtDirectorQt/actionitem.h>
#include <dtDirectorQt/directoreditor.h>
#include <dtDirectorQt/groupitem.h>
#include <dtDirectorQt/macroitem.h>
#include <dtDirectorQt/scriptitem.h>
#include <dtDirectorQt/valueitem.h>
#include <dtDirector/groupnode.h>
#include <dtDirectorNodes/referencescriptaction.h>

#include <QtGui/QDrag>
#include <QtCore/QMimeData>
#include <QtGui/QPainter>
#include <QtGui/QImage>


////////////////////////////////////////////////////////////////////////////////
static const float NODE_BUFFER = 25.0f;

namespace dtDirector
{
   ////////////////////////////////////////////////////////////////////////////////
   NodeScene::NodeScene(DirectorEditor* parent)
      : QGraphicsScene(parent)
      , mpEditor(parent)
      , mpDraggedItem(NULL)
   {
      setBackgroundBrush(Qt::lightGray);

      mpGraph = new DirectorGraph(parent->GetDirector());
   }

   ///////////////////////////////////////////////////////////////////////////////
   void NodeScene::RefreshNodes(NodeType::NodeTypeEnum nodeType)
   {
      // Clear out any previous items and re-add translation item
      clear();
      mpTranslationItem = new QGraphicsRectItem(NULL, this);

      std::vector<const NodeType*> nodes;
      NodeManager::GetInstance().GetNodeTypes(nodes);

      int count = (int)nodes.size();
      float nodeY = 0.0f;
      float maxWidth = 0.0f;
      for (int index = 0; index < count; index++)
      {
         const NodeType* node = nodes[index];

         if (node)
         {
            // Make sure the node we found is a type valid for this script.
            NodePluginRegistry* reg = NodeManager::GetInstance().GetRegistryForType(*node);
            if (!reg || (reg->GetName() != "dtDirectorNodes" && !mpEditor->GetDirector()->HasLibrary(reg->GetName())))
            {
               continue;
            }

            if (node->GetNodeType() == nodeType)
            {
               CreateNode(nodeType, node->GetName(), node->GetCategory(), NODE_BUFFER, nodeY, maxWidth);
               nodeY += NODE_BUFFER;
            }
         }
      }
      QRectF sceneBounds = sceneRect();
      sceneBounds.setHeight(nodeY + NODE_BUFFER);
      sceneBounds.setWidth(maxWidth + NODE_BUFFER);
      setSceneRect(sceneBounds);
   }

   ///////////////////////////////////////////////////////////////////////////////
   void NodeScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
   {
   }

   ///////////////////////////////////////////////////////////////////////////////
   void NodeScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent)
   {
      QGraphicsScene::mouseDoubleClickEvent(mouseEvent);
      QPointF scenePos = mouseEvent->scenePos() - mpTranslationItem->scenePos();
      NodeItem* selectedItem = GetNodeItemAtPos(scenePos);
      if (selectedItem != NULL)
      {
         QString name = selectedItem->QGraphicsItem::data(Qt::UserRole).toString();
         QString category = selectedItem->QGraphicsItem::data(Qt::UserRole + 1).toString();
         emit CreateNode(name, category);
      }
   }

   ///////////////////////////////////////////////////////////////////////////////
   void NodeScene::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
   {
      QPointF scenePos = mouseEvent->scenePos() - mpTranslationItem->scenePos();
      mpDraggedItem = GetNodeItemAtPos(scenePos);

      if (mpDraggedItem == NULL)
      {
         mouseEvent->ignore();
         return;
      }
      
      //we'll render the NodeItem into a Pixmap, so there's something to see while
      //it's being dragged.

      //size of the image to cover the item.  The width is padded to account for 
      //any additional geometry on the left side.
      int imageWidth = mpDraggedItem->boundingRect().width()+mpDraggedItem->boundingRect().height();
      int imageHeight = mpDraggedItem->boundingRect().height();
      
      QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
      image.fill(qRgba(0, 0, 0, 0));

      QPainter painter;
      painter.begin(&image);
      painter.setBrush(mpDraggedItem->brush());
      
      //shift to the right to account for some negative geometry
      painter.translate(mpDraggedItem->boundingRect().height()/2, 0); 

      painter.drawPolygon(mpDraggedItem->polygon(), Qt::WindingFill);
      painter.end();

      QDrag* drag = new QDrag(mouseEvent->widget());
      QMimeData* mime = new QMimeData;
      drag->setMimeData(mime);
      drag->setPixmap(QPixmap::fromImage(image));

      QPoint hotspot(mouseEvent->scenePos().toPoint() - static_cast<QGraphicsItem*>(mpDraggedItem)->scenePos().toPoint());

      drag->setHotSpot(hotspot + QPoint(mpDraggedItem->boundingRect().height()/2.f, 0.f));

      QVariant name = mpDraggedItem->QGraphicsItem::data(Qt::UserRole);
      QVariant category = mpDraggedItem->QGraphicsItem::data(Qt::UserRole + 1);

      QByteArray itemData;
      QDataStream dataStream(&itemData, QIODevice::WriteOnly);
      dataStream <<  name.toString() << category.toString() << QPoint(hotspot);

      //store the name, category, and hotspot data for the upcoming drop event
      mime->setData("data", itemData);
      drag->exec();
      mpDraggedItem = NULL;
   }

   ///////////////////////////////////////////////////////////////////////////////
   void NodeScene::CreateNode(NodeType::NodeTypeEnum nodeType, const std::string& name,
      const std::string& category, float x, float& y, float& maxWidth)
   {
      dtCore::RefPtr<Node> node = NodeManager::GetInstance().CreateNode(name, category, mpGraph);
      if (node.valid())
      {
         NodeItem* item = NULL;
         node->SetPosition(osg::Vec2(x, y));
         switch (nodeType)
         {
            case NodeType::VALUE_NODE:
            {
               item = new ValueItem(node, mpTranslationItem, NULL);
               break;
            }
            case NodeType::MACRO_NODE:
            {
               if (IS_A(node.get(), ReferenceScriptAction*))
               {
                  item = new ScriptItem(node, mpTranslationItem, NULL);
               }
               else
               {
                  item = new MacroItem(NULL, mpTranslationItem, NULL);
               }
               break;
            }
            case NodeType::MISC_NODE:
            {
               if (IS_A(node.get(), GroupNode*))
               {
                  item = new GroupItem(node, mpTranslationItem, NULL, false);
                  break;
               }
            }
            default:
            {
               item = new ActionItem(node, mpTranslationItem, NULL);
               break;
            }
         }

         if (item != NULL)
         {
            item->setFlag(QGraphicsItem::ItemIsMovable, false);
            item->setFlag(QGraphicsItem::ItemIsSelectable, false);
            item->setAcceptedMouseButtons(Qt::NoButton);
            item->setData(Qt::UserRole, QString::fromStdString(name));
            item->setData(Qt::UserRole + 1, QString::fromStdString(category));

#if(QT_VERSION >= 0x00040600)
            item->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
#endif
            item->Draw();
            y += item->boundingRect().height();
            if (item->boundingRect().width() > maxWidth)
            {
               maxWidth = item->boundingRect().width();
            }
         }
      }
   }

   ///////////////////////////////////////////////////////////////////////////////
   NodeItem* NodeScene::GetNodeItemAtPos(const QPointF& pos)
   {
      QList<QGraphicsItem*> nodes = items(pos);
      int count = (int)nodes.size();
      for (int index = 0; index < count; index++)
      {
         NodeItem* nodeItem = dynamic_cast<NodeItem*>(nodes[index]);
         if (nodeItem != NULL)
         {
            return nodeItem;
         }
      }

      return NULL;
   }

} // namespace dtDirector

//////////////////////////////////////////////////////////////////////////