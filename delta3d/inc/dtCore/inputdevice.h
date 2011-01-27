/*
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2004-2011 MOVES Institute
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

#ifndef DELTA_INPUTDEVICE
#define DELTA_INPUTDEVICE

// inputdevice.h: Declaration of the InputDevice class.
//
//////////////////////////////////////////////////////////////////////

#include <dtCore/base.h>
#include <dtCore/export.h>

#include <dtCore/axis.h>
#include <dtCore/button.h>
#include <dtCore/refptr.h>

#include <map>

namespace dtCore
{
   class AxisListener;
   class ButtonListener;
   class InputDeviceFeature;

   /// Represents an input device.
   class DT_CORE_EXPORT InputDevice : public Base
   {
      friend class Button;
      friend class Axis;

      DECLARE_MANAGEMENT_LAYER(InputDevice)

   public:
      typedef std::vector< dtCore::RefPtr<InputDeviceFeature> > FeatureVector;
      typedef std::map< int, dtCore::RefPtr<Button> > ButtonMap;
      typedef std::vector< dtCore::RefPtr<Axis> > AxisVector;

      /**
       * Constructor.
       *
       * @param name the instance name
       */
      InputDevice(const std::string& name = "InputDevice");

   protected:

      /**
       * Destructor.
       */
      virtual ~InputDevice();

   public:

      /**
       * Returns the number of features (buttons, axes, etc.) contained in
       * this device.
       *
       * @return the number of features contained in this device
       */
      int GetFeatureCount() const;

      /**
       * Returns a pointer to the specified feature.
       *
       * @param index the index of the desired feature
       * @return a pointer to the feature
       */
      InputDeviceFeature* GetFeature(int index);

      /**
       * Returns the number of features (buttons, axes, etc.) contained in
       * this device.
       *
       * @return the number of features contained in this device
       */
      int GetButtonCount() const;

      /**
       * Returns a pointer to the specified feature.
       *
       * @param index the index of the desired feature
       * @return a pointer to the feature
       */
      Button* GetButton(int index);
      const Button* GetButton(int index) const;

      /**
       * Get the index in the array of buttons for given button.
       *
       * @param button the button whose index is to be found
       * @return the index of given button if valid, else -1
       */
      int GetButtonIndex(const Button* button) const;

      /**
       * Returns the number of features (buttons, axes, etc.) contained in
       * this device.
       *
       * @return the number of features contained in this device
       */
      int GetAxisCount() const;

      /**
       * Returns a pointer to the specified feature.
       *
       * @param index the index of the desired feature
       * @return a pointer to the feature
       */
      Axis* GetAxis(int index);
      const Axis* GetAxis(int index) const;

      /**
       * Get the index in the array of axes for given axis.
       *
       * @param axis the axis whose index is to be found
       * @return the index of given axis if valid, else -1
       */
      int GetAxisIndex(const Axis* axis) const;

      /**
       * Adds a button listener.
       *
       * @param buttonListener a pointer to the listener to add
       */
      void AddButtonListener(ButtonListener* buttonListener);

      /**
       * Removes a button listener.
       *
       * @param buttonListener a pointer to the listener to remove
       */
      void RemoveButtonListener(ButtonListener* buttonListener);

      /**
       * Adds an axis listener.
       *
       * @param axisListener a pointer to the listener to add
       */
      void AddAxisListener(AxisListener* axisListener);

      /**
       * Removes an axis listener.
       *
       * @param axisListener a pointer to the listener to remove
       */
      void RemoveAxisListener(AxisListener* axisListener);

      /// Adds a feature to this device.
      /// @param feature a pointer to the feature to add
      bool AddFeature(InputDeviceFeature* feature);

      /// Removes a feature from this device.
      /// @param feature a pointer to the feature to remove
      void RemoveFeature(InputDeviceFeature* feature);

      const ButtonMap& GetButtons() const { return mButtons; }
      ButtonMap& GetButtons() { return mButtons; }

      const AxisVector& GetAxes() const { return mAxes; }
      AxisVector& GetAxes() { return mAxes; }

   private:
      FeatureVector mFeatures;  ///< The list of features.
      ButtonMap mButtons;  ///< The list of buttons.
      AxisVector mAxes;  ///< The list of axes.

      typedef std::list<ButtonListener*> ButtonListenerList; ///< A container of ButtonListeners.
      typedef std::list<AxisListener*> AxisListenerList;     ///< A container of AxisListeners.

      ButtonListenerList mButtonListeners;  ///< The container of ButtonListeners.
      AxisListenerList mAxisListeners;      ///< The container of AxisListeners.
   };
}

#endif // DELTA_INPUTDEVICE
