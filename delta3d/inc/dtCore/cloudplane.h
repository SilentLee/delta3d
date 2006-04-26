/* 
 * Delta3D Open Source Game and Simulation Engine 
 * Copyright (C) 2004-2005 MOVES Institute 
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
*/

#ifndef DELTA_CLOUDPLANE
#define DELTA_CLOUDPLANE


#include "dtCore/enveffect.h"
#include "dtCore/refptr.h"

#include <osg/Vec2>
#include <osg/Vec4>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Transform>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osgUtil/CullVisitor>
#include <osg/Fog>

namespace dtCore
{
   /**
   * CloudPlane:  This class can be used to simulate cloud cover
   *                 it derives from EnvEffect
   *
   */
   class DT_CORE_EXPORT CloudPlane : public dtCore::EnvEffect
   {
   public:
      
       DECLARE_MANAGEMENT_LAYER(CloudPlane)

       /**
       * Constructor: These params initialize the noise function
       * for a detailed description see dtUtil::Fractal   
       *
       *@sa dtUtil::Fractal
       */
       CloudPlane(int  octaves,
                 float cutoff,
                 int   frequency,
                 float amp,
                 float persistence,
                 float density,
                 int   texSize,
                 float height,
                 const std::string& name = "CloudPlane");
      
      ~CloudPlane();

      ///the virtual overloaded draw function
      virtual void Repaint(   osg::Vec4 sky_color, osg::Vec4 fog_color, 
                              double sun_angle, double sunAzimuth,
                              double vis);

      ///@return height of skyplane
      float GetHeight() { return mHeight; };

   private:
       class MoveEarthySkyWithEyePointTransform : public osg::Transform
       {
       public:

           /** Get the transformation matrix which moves from local coords to world coords.*/
           virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const 
           {
               osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
               if (cv)
               {
                   osg::Vec3 eyePointLocal = cv->getEyeLocal();
                   matrix.preMult(osg::Matrix::translate(eyePointLocal.x(),eyePointLocal.y(),eyePointLocal.z()));
               }
               return true;
           }

           /** Get the transformation matrix which moves from world coords to local coords.*/
           virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const
           {    
               osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
               if (cv)
               {
                   osg::Vec3 eyePointLocal = cv->getEyeLocal();
                   matrix.postMult(osg::Matrix::translate(-eyePointLocal.x(),-eyePointLocal.y(),-eyePointLocal.z()));
               }
               return true;
           }
       };

       void Create( void );
       osg::Texture2D *createPerlinTexture(void);
       virtual void OnMessage(MessageData *data);
       void Update(const double deltaFrameTime);
       osg::Geometry *createPlane(float, float);

       //dtCore::RefPtr<osg::Group> mNode;
       dtCore::RefPtr<osg::Geode> mGeode;
       dtCore::RefPtr<osg::Geometry> mPlane;
       dtCore::RefPtr<osg::Image> mImage;
       dtCore::RefPtr<osg::Texture2D> mCloudTexture;
       dtCore::RefPtr<osg::Fog> mFog; ///< The fog adjuster
       int mOctaves;
       float mCutoff;
       int   mFrequency;
       float mAmplitude;
       float mPersistence;
       float mDensity;
       float mHeight;
       int   mTexSize;
       osg::Vec4 mSkyColor;
       osg::Vec2 *mWind;
       osg::Vec4 *mCloudColor;
       osg::Vec2Array *mTexCoords;
       osg::Vec4Array *mColors;
       dtCore::RefPtr<MoveEarthySkyWithEyePointTransform> mXform;
       osg::ref_ptr<osg::TexMat> mTexMat;

   };

}


#endif // DELTA_CLOUDPLANE
