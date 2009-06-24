#ifndef LINKED_POINT_ARRAY_ACTOR_PLUGIN
#define LINKED_POINT_ARRAY_ACTOR_PLUGIN

#include "export.h"
#include <dtEditQt/mainwindow.h>
#include <dtEditQt/plugininterface.h>
#include <QtGui/QPushButton>
#include <QtGui/QCheckBox>
#include <QtGui/QDockWidget>
#include <dtActors/linkedpointsactorproxy.h>
#include "toolobjectmotionmodel.h"

using namespace dtEditQt;

namespace dtEditQt
{
   class Viewport;
}

/**
 * The LinkedPointsActorToolPlugin is a plugin that is used as a tool
 * to place a LinkedPointsActor into the world.
 */
class DT_LINKED_POINTS_ACTOR_EXPORT LinkedPointsActorToolPlugin 
   : public QDockWidget
   , public Plugin
{
   Q_OBJECT

public:

   const static std::string PLUGIN_NAME;
   const static std::string MODE_ICON;
   
   LinkedPointsActorToolPlugin(MainWindow* mw);

   ~LinkedPointsActorToolPlugin();

   void InitializeMotionModels();

   virtual void Destroy();

   /** override close event to get notified when user closes the dock */
   virtual void closeEvent(QCloseEvent* event);

public slots:

   /**
   * Handles when a new actor proxy is created.
   *
   * @param[in]  proxy               The proxy that was created.
   * @param[in]  forceNoAdjustments  If true, all slots catching this event should not change the proxy.
   */
   void onActorProxyCreated(ActorProxyRefPtr proxy, bool forceNoAdjustments);

   /**
   * Handles when an actor has been destroyed.
   *
   * @param[in]  proxy  The proxy being destroyed.
   */
   void onActorProxyDestroyed(ActorProxyRefPtr proxy);

   /**
   * Handles when actors are selected.
   *
   * @param[in]  actors  The list of actors being selected.
   */
   void onActorsSelected(ActorProxyRefPtrVector& actors);

   /**
   * Signal used when a mouse has been pressed in a viewport.
   *
   * @param[in]  vp  The viewport triggering this event.
   * @param[in]  e   The mouse event.
   */
   void onMousePressEvent(Viewport* vp, QMouseEvent* e);

   /**
   * Signal used when a mouse has been released in a viewport.
   *
   * @param[in]  vp  The viewport triggering this event.
   * @param[in]  e   The mouse event.
   */
   void onMouseReleaseEvent(Viewport* vp, QMouseEvent* e);

   /**
   * Signal used when a mouse has been moved in a viewport.
   *
   * @param[in]  vp  The viewport triggering this event.
   * @param[in]  e   The mouse event.
   */
   void onMouseMoveEvent(Viewport* vp, QMouseEvent* e);

   /**
   * Signal used when testing whether actor mode should be activated.
   *
   * @param[in]   vp               The viewport triggering this event.
   * @param[in]   position         The position of the mouse click.
   * @param[out]  overrideDefault  Should be set true if you don't want the default behavior to handle this.
   * @param[out]  result           The result of the test if it is overridden.
   */
   void onShouldBeginActorMode(Viewport* vp, osg::Vec2 position, bool* overrideDefault, bool* result);

   /**
   * Signal used when camera mode is begun.
   *
   * @param[in]   vp               The viewport triggering this event.
   * @param[in]   e                The mouse event.
   * @param[out]  overrideDefault  Should be set true if you don't want the default behavior to handle this.
   */
   void onBeginCameraMode(Viewport* vp, QMouseEvent* e, bool* overrideDefault = NULL);

   /**
   * Signal used when camera mode is ended.
   *
   * @param[in]   vp               The viewport triggering this event.
   * @param[in]   e                The mouse event.
   * @param[out]  overrideDefault  Should be set true if you don't want the default behavior to handle this.
   */
   void onEndCameraMode(Viewport* vp, QMouseEvent* e, bool* overrideDefault = NULL);

   /**
   * Handles when actor mode is begun.
   *
   * @param[in]   vp               The viewport triggering this event.
   * @param[in]   e                The mouse event.
   * @param[out]  overrideDefault  Should be set true if you don't want the default behavior to handle this.
   */
   void onBeginActorMode(Viewport* vp, QMouseEvent* e, bool* overrideDefault);

   /**
   * Handles when actor mode is ended.
   *
   * @param[in]   vp               The viewport triggering this event.
   * @param[in]   e                The mouse event.
   * @param[out]  overrideDefault  Should be set true if you don't want the default behavior to handle this.
   */
   void onEndActorMode(Viewport* vp, QMouseEvent* e, bool* overrideDefault);

   /**
   * Handles when to select actors.
   *
   * @param[in]   vp               The viewport triggering this event.
   * @param[in]   e                The mouse event.
   * @param[out]  overrideDefault  Should be set true if you don't want the default behavior to handle this.
   */
   void onSelectActors(Viewport* vp, QMouseEvent* e, bool* overrideDefault);

   /**
   * Handles when duplicating the current selection.
   *
   * @param[in]   vp               The viewport triggering this event.
   * @param[out]  overrideDefault  Should be set true if you don't want the default behavior to handle this.
   */
   void onDuplicateActors(Viewport* vp, bool* overrideDefault);

   /**
   * Puts the perspective viewport options in sync with the editor preferences.
   */
   void onEditorPreferencesChanged();

   /**
   * Sets the translation snap increment.
   *
   * @param[in]  increment  The size of the increment.
   */
   void onSetSnapTranslation(float increment);

   /**
   * Sets the angular snap increment.
   *
   * @param[in]  increment  The size of the increment.
   */
   void onSetSnapRotation(float increment);

   /**
   * Sets the scalar snap increment.
   *
   * @param[in]  increment  The size of the increment.
   */
   void onSetSnapScale(float increment);

   /**
   * Sets the enabled status of each snap tool.
   */
   void onSetSnapEnabled(bool translation, bool rotation, bool scale);

   /**
   * User has pressed the mode button.
   */
   void onModeButtonPressed();

   /**
   * User has pressed the Creation Mode Checkbox.
   */
   void onCreationModePressed();

   /**
   * User has pressed the delete link point button.
   */
   void onDeleteLinkPointPressed();

   /**
   * User has pressed the finish button.
   */
   void onFinishedButtonPressed();

private:

   /**
   * Initializes the tool.
   */
   void initialize(dtActors::LinkedPointsActorProxy* activeProxy);

   /**
   * Shuts down the tool.
   */
   void shutdown();

   /**
   * Sets the given drawable as the selected, if it belongs to the active proxy.
   *
   * @param[in]  drawable  The drawable to select.
   *
   * @return     True if the drawable belongs to the active proxy.
   */
   bool selectDrawable(dtCore::DeltaDrawable* drawable);

   /**
   * Retrieves the appropriate tool motion model based on the given viewport.
   *
   * @param[in]  vp  The viewport.
   *
   * @retun      The tool object motion model that belongs to the viewport.
   */
   ToolObjectMotionModel* GetMotionModelForView(Viewport* vp);

   /**
   * Shows or Hides the placement ghost.
   */
   void ShowPlacementGhost();
   void HidePlacementGhost();
   void UpdatePlacementGhost(Viewport* vp, osg::Vec2 mousePos);

   MainWindow* mMainWindow;

   bool        mIsDocked;
   bool        mIsActive;
   bool        mShowingPlacementGhost;

   bool        mIsInActorMode;
   bool        mIsInCameraMode;

   int         mCurrentPoint;

   dtCore::RefPtr<dtActors::LinkedPointsActorProxy> mActiveProxy;
   dtActors::LinkedPointsActor*                     mActiveActor;

   dtCore::RefPtr<ToolObjectMotionModel>            mPerspMotionModel;
   dtCore::RefPtr<ToolObjectMotionModel>            mTopMotionModel;
   dtCore::RefPtr<ToolObjectMotionModel>            mSideMotionModel;
   dtCore::RefPtr<ToolObjectMotionModel>            mFrontMotionModel;

   QAction*     mModeButton;

   QCheckBox*   mCreationModeCheckbox;
   QPushButton* mDeleteLinkButton;
   QPushButton* mFinishedButton;
};

#endif //LINKED_POINT_ARRAY_ACTOR_PLUGIN
