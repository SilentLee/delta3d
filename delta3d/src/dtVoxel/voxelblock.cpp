/* -*-c++-*-
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2015, Caper Holdings, LLC
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
 */

#include <dtVoxel/voxelblock.h>
#include <dtUtil/log.h>
#include <dtUtil/fileutils.h>
#include <dtUtil/stringutils.h>
#include <dtCore/project.h>

#include <osgUtil/Simplifier>
#include <osgUtil/Optimizer>

#include <osgDB/WriteFile>
#include <osg/PolygonMode>
#include <osg/PagedLOD>

#include <tbb/parallel_for.h>
#include <iostream>

namespace dtVoxel
{

   VoxelBlock::VoxelBlock()
      : mIsAllocated(false)      
      , mIsDirty(false)
      , mIsEmpty(true)
      , mNumCells(0)
      , mCells(NULL)
   {
   }

   VoxelBlock::~VoxelBlock()
   {
   }


   void VoxelBlock::Init(const osg::Vec3& ws_dimensions, const osg::Vec3& ws_offset, const osg::Vec3f& cellSize)
   {
      //static int count = 0;
      //std::cout << "Creating Voxel Block " << count++ << std::endl;

      mWSDimensions = ws_dimensions;
      mOffset = ws_offset;
      mWSCellDimensions = cellSize;

      mVolume = new osg::Group();
   }
   
   void VoxelBlock::DeAllocate()
   {
      //std::cout << "DeAllocating Voxel Block" << std::endl;

      if (mVolume.valid())
      {
         mVolume->removeChildren(0, mVolume->getNumChildren());

         if (mCells != nullptr && IsAllocated())
         {
            for (int i = 0; i < mNumCells; ++i)
            {
               mCells[i].DeAllocate();
            }

            delete[] mCells;
            mCells = NULL;
         }
      }

      mIsAllocated = false;
   }

   bool VoxelBlock::IsAllocated() const
   {
      return mIsAllocated;
   }

   void VoxelBlock::SetDirty(bool b)
   {
      mIsDirty = b;
   }

   bool VoxelBlock::GetDirty() const
   {
      return mIsDirty;
   }

   void VoxelBlock::SetEmpty(bool b)
   {
      mIsEmpty = b;
   }

   bool VoxelBlock::GetEmpty() const
   {
      return mIsEmpty;
   }

   const osg::Vec3& VoxelBlock::GetOffset()
   {
      return mOffset;
   }

   void VoxelBlock::RegenerateAABB(VoxelActor& voxelActor, const osg::BoundingBox& bb, const osg::Vec3i& textureResolution)
   {
      osg::Vec3 newDim;
      newDim[0] = bb.xMax() - bb.xMin();
      newDim[1] = bb.yMax() - bb.yMin();
      newDim[2] = bb.zMax() - bb.zMin();

      int blocksX = int(std::floor(newDim.x() / mWSCellDimensions.x()));
      int blocksY = int(std::floor(newDim.y() / mWSCellDimensions.y()));
      int blocksZ = int(std::floor(newDim.z() / mWSCellDimensions.z()));

      //std::cout << "Blocks X " << blocksX << " Blocks Y " << blocksY << " Blocks Z " << blocksZ << std::endl;

      osg::Vec3 bboffset = bb._min - mOffset;
      int startX = int(std::floor(bboffset.x() / mWSDimensions.x()));
      int startY = int(std::floor(bboffset.y() / mWSDimensions.y()));
      int startZ = int(std::floor(bboffset.z() / mWSDimensions.z()));

      //std::cout << "start X " << startX << " start Y " << startY << " start Z " << startZ << std::endl;

      int endX = startX + blocksX;
      int endY = startY + blocksY;
      int endZ = startZ + blocksZ;

      for (int z = startZ; z < endZ; z++)
      {
         for (int y = startY; y < endY; y++)
         {
            for (int x = startX; x < endX; x++)
            {
               VoxelCell* vc = GetCellFromIndex(x, y, z);
               if (vc != nullptr)
               {
                  osg::Vec3 pos(x * mWSCellDimensions[0], y * mWSCellDimensions[1], z * mWSCellDimensions[2]);
                  osg::Vec3 offsetFrom = pos + mOffset;
                  osg::Vec3 offsetTo = offsetFrom + mWSCellDimensions;

                  osg::Matrix transform;
                  transform.makeScale(1.0f, 1.0f, 1.0f); //compute offset
                  transform.setTrans(offsetFrom);

                  //todo using local grid should be faster
                  osg::BoundingBox bb(offsetFrom, offsetTo);
                  openvdb::GridBase::Ptr vdbGrid = voxelActor.CollideWithAABB(bb);
                  
                  if (mVolume->getNumChildren() > 0)
                  {
                     osg::LOD* lod = dynamic_cast<osg::LOD*>(mVolume->getChild(0));
                     if (lod != nullptr)
                     {
                        osg::Group* lodChild0 = lod->getChild(0)->asGroup();
               
                        if (vc->IsAllocated())
                        {
                           lodChild0->removeChild(vc->GetOSGNode());
                        }

                        if (vdbGrid != NULL && !vdbGrid->empty())
                        {
                           vc->CreateMesh(voxelActor, vdbGrid, transform, mWSCellDimensions, textureResolution);
                           lodChild0->addChild(vc->GetOSGNode());
                        }
                     }
                     else
                     {
                        LOG_ERROR("Error finding LOD node, cannot regenerate mesh.");
                     }
                  }
                  else
                  {
                     LOG_ERROR("Voxel Block is unallocated, cannot regenerate mesh.");
                  }
               }
               else
               {
                  LOG_ERROR("Error calculating VoxelCell AABB region");
               }
            }
         }
      }

      SetDirty(false);
   }

   void VoxelBlock::AllocateCells(VoxelActor& voxelActor, osg::Group& parentNode, const osg::Vec3& gridDimensions, const osg::Vec3i& textureResolution)
   {
      std::cout << "Allocating Individual Voxel Block Cells" << std::endl;
           
      for (unsigned int z = 0; z < gridDimensions[2]; ++z)
      {
         for (unsigned int y = 0; y < gridDimensions[1]; ++y)
         {
            for (unsigned int x = 0; x < gridDimensions[0]; ++x)
            {
               osg::Vec3 pos(x * mWSCellDimensions[0], y * mWSCellDimensions[1], z * mWSCellDimensions[2]);
               osg::Vec3 offsetFrom = pos + mOffset;
               osg::Vec3 offsetTo = offsetFrom + mWSCellDimensions;

               osg::Matrix transform;
               transform.makeScale(1.0f, 1.0f, 1.0f); //compute offset
               transform.setTrans(offsetFrom);

               //todo using local grid should be faster
               osg::BoundingBox bb(offsetFrom, offsetTo);
               openvdb::GridBase::Ptr vdbGrid = voxelActor.CollideWithAABB(bb);
               //bool collides = voxelActor.FastSampleWithAABB(bb, osg::Vec3i(25, 25, 10));

               if (vdbGrid != NULL && !vdbGrid->empty())
               {
                  //keeping a pointer and incrementing it should be much faster for a large contiguous dataset
                  int cellIndex = (z * gridDimensions[1] * gridDimensions[0]) + (y * gridDimensions[0]) + x;
                  
                  mCells[cellIndex].CreateMesh(voxelActor, vdbGrid, transform, mWSCellDimensions, textureResolution);

                  //todo- should these be spatialized?
                  parentNode.addChild(mCells[cellIndex].GetOSGNode());
               }

               //++cellToInit;
            }
         }
      }

      //simplify mesh
      //dtCore::RefPtr<osgUtil::Simplifier> simplifier = new osgUtil::Simplifier();
      //simplifier->setMaximumLength(1000.0f);
      //simplifier->setDoTriStrip(true);
      //parentNode.accept(*simplifier);

      /*spatialize
      osgUtil::Optimizer opt;
      opt.optimize(&parentNode, osgUtil::Optimizer::SPATIALIZE_GROUPS);*/

      mIsAllocated = true;
   }

   void VoxelBlock::AllocateLODMesh(VoxelActor& voxelActor, const osg::Vec3i& resolution0, float dist0, const osg::Vec3i& resolution1, float dist1, const osg::Vec3i& resolution2, float dist2, const osg::Vec3i& resolution3, float viewDistance)
   {
      osg::Vec3 gridDim(
         1.0 + int(std::floor(mWSDimensions[0] / mWSCellDimensions[0])),
         1.0 + int(std::floor(mWSDimensions[1] / mWSCellDimensions[1])),
         1.0 + int(std::floor(mWSDimensions[2] / mWSCellDimensions[2])));

      mNumCells = gridDim[0] * gridDim[1] * gridDim[2];
      //todo catch out of memory exception here
      mCells = new VoxelCell[mNumCells];


      dtCore::RefPtr<osg::LOD> lodNode = new osg::LOD();

      dtCore::RefPtr<osg::Group> node0 = new osg::Group;
      dtCore::RefPtr<osg::Group> node1 = new osg::Group;
      dtCore::RefPtr<osg::Group> node2 = new osg::Group;
      dtCore::RefPtr<osg::Group> node3 = new osg::Group;

          
      //AllocateCells(voxelActor, *node0, gridDim, resolution0);
      AllocateCombinedMesh(voxelActor, *node0, gridDim, resolution0);
      AllocateCombinedMesh(voxelActor, *node1, gridDim, resolution1);
      AllocateCombinedMesh(voxelActor, *node2, gridDim, resolution2);
      AllocateCombinedMesh(voxelActor, *node3, gridDim, resolution3);
      

      lodNode->addChild(node0, 0.0f, dist0);
      lodNode->addChild(node1, dist0, dist1);
      lodNode->addChild(node2, dist1, dist2);
      lodNode->addChild(node3, dist2, viewDistance);

      mVolume->addChild(lodNode);
      
      mIsAllocated = true;
   }

   void VoxelBlock::WritePagedLOD(VoxelActor& voxelActor, int index, const std::string& filePath, const osg::Vec3i& resolution0, float dist0, const osg::Vec3i& resolution1, float dist1, const osg::Vec3i& resolution2, float dist2, const osg::Vec3i& resolution3, float viewDistance)
   {
      osg::Vec3 gridDim(
         1.0 + int(std::floor(mWSDimensions[0] / mWSCellDimensions[0])),
         1.0 + int(std::floor(mWSDimensions[1] / mWSCellDimensions[1])),
         1.0 + int(std::floor(mWSDimensions[2] / mWSCellDimensions[2])));

      mNumCells = gridDim[0] * gridDim[1] * gridDim[2];
      //todo catch out of memory exception here
      mCells = new VoxelCell[mNumCells];


      dtCore::RefPtr<osg::PagedLOD> lodNode = new osg::PagedLOD();
      lodNode->setDatabasePath(filePath);
      lodNode->setRadius(2.0 * mWSDimensions.length());
      lodNode->setCenter(mOffset + (mWSDimensions * 0.5f));

      dtCore::RefPtr<osg::Group> node0 = new osg::Group;
      dtCore::RefPtr<osg::Group> node1 = new osg::Group;
      dtCore::RefPtr<osg::Group> node2 = new osg::Group;
      dtCore::RefPtr<osg::Group> node3 = new osg::Group;


      //AllocateCells(voxelActor, *node0, gridDim, resolution0);
      AllocateCombinedMesh(voxelActor, *node0, gridDim, resolution0);      
      std::string fileName = SaveCachedModel(filePath, *node0, index, 0);
      if (!fileName.empty())
      {
         lodNode->setFileName(0, fileName);
         lodNode->setRange(0, 0.0f, dist0);
      }
      else
      {
         LOG_ERROR("Error writing paged lod node 0.");
      }

      AllocateCombinedMesh(voxelActor, *node1, gridDim, resolution1);      
      fileName = SaveCachedModel(filePath, *node1, index, 1);
      if (!fileName.empty())
      {
         lodNode->setFileName(1, fileName);
         lodNode->setRange(1, dist0, dist1);
      }
      else
      {
         LOG_ERROR("Error writing paged lod node 1.");
      }

      AllocateCombinedMesh(voxelActor, *node2, gridDim, resolution2);      
      fileName = SaveCachedModel(filePath, *node2, index, 2);
      if (!fileName.empty())
      {
         lodNode->setFileName(2, fileName);
         lodNode->setRange(2, dist1, dist2);
      }
      else
      {
         LOG_ERROR("Error writing paged lod node 2.");
      }

      AllocateCombinedMesh(voxelActor, *node3, gridDim, resolution3);
      fileName = SaveCachedModel(filePath, *node3, index, 3);
      if (!fileName.empty())
      {
         lodNode->setFileName(3, fileName);
         lodNode->setRange(3, dist2, viewDistance);
      }
      else
      {
         LOG_ERROR("Error writing paged lod node 3.");
      }


      mVolume->addChild(lodNode);

      SaveCachedModel(filePath, index);

      mIsAllocated = true;
   }


   void VoxelBlock::AllocateCombinedMesh(VoxelActor& voxelActor, osg::Group& parentNode, const osg::Vec3& gridDimensions, const osg::Vec3i& textureResolution)
   {
      std::cout << "Allocating Combined Voxel Block" << std::endl;

      dtCore::RefPtr<osg::Geometry> geom = new osg::Geometry();
      dtCore::RefPtr<osg::Vec3Array> vertArray = new osg::Vec3Array();
      dtCore::RefPtr<osg::Vec3Array> normalArray = new osg::Vec3Array();
      dtCore::RefPtr<osg::Vec3Array> colorArray = new osg::Vec3Array();
      dtCore::RefPtr<osg::DrawElementsUInt> drawElements = new osg::DrawElementsUInt(GL_TRIANGLES);

      //VoxelCell* cellToInit = &mCells[0];
      for (unsigned int z = 0; z < gridDimensions[2]; ++z)
      {
         for (unsigned int y = 0; y < gridDimensions[1]; ++y)
         {
            for (unsigned int x = 0; x < gridDimensions[0]; ++x)
            {
               osg::Vec3 pos(x * mWSCellDimensions[0], y * mWSCellDimensions[1], z * mWSCellDimensions[2]);
               osg::Vec3 offsetFrom = pos + mOffset;
               osg::Vec3 offsetTo = offsetFrom + mWSCellDimensions;

               osg::Matrix transform;
               transform.makeScale(1.0f, 1.0f, 1.0f); //compute offset
               transform.setTrans(offsetFrom);

               //todo using local grid should be faster
               osg::BoundingBox bb(offsetFrom, offsetTo);
               openvdb::GridBase::Ptr vdbGrid = voxelActor.CollideWithAABB(bb);
               //bool collides = voxelActor.FastSampleWithAABB(bb, osg::Vec3i(25, 25, 10));

               if (vdbGrid != NULL && !vdbGrid->empty())
               {
                  //keeping a pointer and incrementing it should be much faster for a large contiguous dataset
                  int cellIndex = (z * gridDimensions[1] * gridDimensions[0]) + (y * gridDimensions[0]) + x;
                  mCells[cellIndex].AddGeometry(voxelActor, vdbGrid, transform, mWSCellDimensions, textureResolution, vertArray, normalArray, colorArray, drawElements);
               }

               //++cellToInit;
            }
         }
      }


      geom->setVertexArray(vertArray);
      geom->setNormalArray(normalArray);
      geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

      geom->addPrimitiveSet(drawElements);

      osg::Geode* geode = new osg::Geode();
      geode->addDrawable(geom);
      parentNode.addChild(geode);      

      /*dtCore::RefPtr<osgUtil::Simplifier> simplifier = new osgUtil::Simplifier();
      simplifier->setSampleRatio(0.2);
      simplifier->setDoTriStrip(true);
      parentNode.accept(*simplifier);
      
      osgUtil::Optimizer opt;
      opt.optimize(&parentNode, osgUtil::Optimizer::MAKE_FAST_GEOMETRY);*/


      osg::StateSet* ss = parentNode.getOrCreateStateSet();

      /*osg::ref_ptr<osg::PolygonMode> polymode = new osg::PolygonMode;
      polymode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
      ss->setAttributeAndModes(polymode.get(), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);*/

      /*ss->setMode(GL_LIGHTING, osg::StateAttribute::OVERRIDE | osg::StateAttribute::OFF);*/

      
      mIsAllocated = true;
   }



   VoxelCell* VoxelBlock::GetCellFromIndex(int x, int y, int z)
   {
      VoxelCell* cell = NULL;

      osg::Vec3 gridDim(
         int(std::floor(mWSDimensions[0] / mWSCellDimensions[0])),
         int(std::floor(mWSDimensions[1] / mWSCellDimensions[1])),
         int(std::floor(mWSDimensions[2] / mWSCellDimensions[2])));

      int numCells = gridDim[0] * gridDim[1] * gridDim[2];
      int cellIndex = (z * gridDim[1] * gridDim[0]) + (y * gridDim[0]) + x;

      if (cellIndex >= 0 && cellIndex < numCells)
      {
         cell = &mCells[cellIndex];
      }

      return cell;
   }

   VoxelCell* VoxelBlock::GetCellFromPos(const osg::Vec3& pos)
   {
      VoxelCell* cell = NULL;

      osg::Vec3 localPos = pos - mOffset;

      int indexX = int(std::floor(localPos[0] / mWSCellDimensions[0]));
      int indexY = int(std::floor(localPos[1] / mWSCellDimensions[1]));
      int indexZ = int(std::floor(localPos[2] / mWSCellDimensions[2]));

      osg::Vec3 gridDim(
         int(std::floor(mWSDimensions[0] / mWSCellDimensions[0])),
         int(std::floor(mWSDimensions[1] / mWSCellDimensions[1])),
         int(std::floor(mWSDimensions[2] / mWSCellDimensions[2])));

      int numCells = gridDim[0] * gridDim[1] * gridDim[2];
      int cellIndex = (indexZ * gridDim[1] * gridDim[0]) + (indexY * gridDim[0]) + indexX;

      if (cellIndex > 0 && cellIndex < numCells)
      {
         cell = &mCells[cellIndex];
      }

      return cell;
   }

   osg::Group* VoxelBlock::GetOSGNode()
   {
      return mVolume.get();
   }

   const osg::Group* VoxelBlock::GetOSGNode() const
   {
      return mVolume.get();
   }

   const osg::Vec3& VoxelBlock::GetOffset() const
   {
      return mOffset;
   }

   bool VoxelBlock::HasCachedModel(const std::string& folderName, int index)
   {
      bool result = false;

      std::string filePath = dtCore::Project::GetInstance().GetContext() + "/" + folderName + "/";

      std::string indexString;
      std::stringstream fileName;

      dtUtil::MakeIndexString(index, indexString, 8);

      fileName << filePath << "VoxelGrid_cache" << indexString << "_paged.osgt";

      if (dtUtil::FileUtils::GetInstance().FileExists(fileName.str()))
      {
         result = true;
      }

      return result;
   }

   bool VoxelBlock::LoadCachedModel(const std::string& folderName, int index)
   {
      bool result = false;

      std::string filePath = dtCore::Project::GetInstance().GetContext() + "/" + folderName + "/";

      std::string indexString;
      std::stringstream fileName;

      dtUtil::MakeIndexString(index, indexString, 8);

      fileName << filePath << "VoxelGrid_cache" << indexString << "_paged.osgt";

      if (dtUtil::FileUtils::GetInstance().FileExists(fileName.str()))
      {
         std::cout << "Reading block num " << index << " from model cache " << fileName.str() << std::endl;

         osg::Node* n = dtUtil::FileUtils::GetInstance().ReadNode(fileName.str());
         if (n != nullptr)
         {
            mVolume->addChild(n);
               
            mIsAllocated = true;
            result = true;
         }
         else
         {
            LOG_ERROR("Error reading cached node.");
         }
         
      }

      return result;
   }



   bool VoxelBlock::SaveCachedModel(const std::string& folderName, int index)
   {
      bool result = false;

      std::string filePath = dtCore::Project::GetInstance().GetContext() + "/" + folderName + "/";

      std::string indexString;
      std::stringstream fileName;

      dtUtil::MakeIndexString(index, indexString, 8);

      fileName << filePath << "VoxelGrid_cache" << indexString << "_paged.osgt";

      if (dtUtil::FileUtils::GetInstance().DirExists(filePath))
      {
         result = osgDB::writeNodeFile(*mVolume, fileName.str());

         std::cout << "Writing PagedLOD for block num " << index << " to model cache " << fileName.str() << std::endl;
      }

      return result;
   }

   std::string VoxelBlock::SaveCachedModel(const std::string& folderName, osg::Node& n, int index, int lod)
   {
      std::string result;

      std::string filePath = dtCore::Project::GetInstance().GetContext() + "/" + folderName + "/";

      std::string indexString;
      std::string lodString;
      std::stringstream fileName;

      dtUtil::MakeIndexString(index, indexString, 8);
      dtUtil::MakeIndexString(lod, lodString, 3);

      fileName << "VoxelGrid_cache" << indexString << "_lod" << lodString << ".osgb";

      if (dtUtil::FileUtils::GetInstance().DirExists(filePath))
      {
         bool success = osgDB::writeNodeFile(n, filePath + fileName.str());

         if (success)
         {
            //result will be empty string if this fails
            result = fileName.str();
            std::cout << "Writing lod " << lod << " for block num " << index << " to model   cache " << filePath + fileName.str() << std::endl;
         }
         else
         {
            LOG_ERROR("Error writing block to disk.");
         }

      }

      return result;
   }


} /* namespace dtVoxel */