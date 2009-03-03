#include <dtInspectorQt/lightmanager.h>
#include "ui_dtinspectorqt.h"
#include <QtGui/QColorDialog>

//////////////////////////////////////////////////////////////////////////
dtInspectorQt::LightManager::LightManager(Ui::InspectorWidget& ui)
:mUI(&ui)
{
   connect(mUI->lightEnabledToggle, SIGNAL(stateChanged(int)), this, SLOT(OnEnabled(int)));
   connect(mUI->lightNumberEdit, SIGNAL(valueChanged(int)), this, SLOT(OnLightNumberChanged(int)));
   connect(mUI->lightModeCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(OnModeChanged(const QString&)));
   connect(mUI->lightAmbRedEdit, SIGNAL(valueChanged(double)), this, SLOT(OnAmbientChanged(double)));
   connect(mUI->lightAmbGreenEdit, SIGNAL(valueChanged(double)), this, SLOT(OnAmbientChanged(double)));
   connect(mUI->lightAmbBlueEdit, SIGNAL(valueChanged(double)), this, SLOT(OnAmbientChanged(double)));
   connect(mUI->lightAmbPushbutton, SIGNAL(clicked()), this, SLOT(OnAmbientColorChooserClicked()));
   connect(mUI->lightDiffRedEdit, SIGNAL(valueChanged(double)), this, SLOT(OnDiffuseChanged(double)));
   connect(mUI->lightDiffGreenEdit, SIGNAL(valueChanged(double)), this, SLOT(OnDiffuseChanged(double)));
   connect(mUI->lightDiffBlueEdit, SIGNAL(valueChanged(double)), this, SLOT(OnDiffuseChanged(double)));
   connect(mUI->lightDiffPushbutton, SIGNAL(clicked()), this, SLOT(OnDiffuseColorChooserClicked()));
   connect(mUI->lightSpecRedEdit, SIGNAL(valueChanged(double)), this, SLOT(OnSpecularChanged(double)));
   connect(mUI->lightSpecGreenEdit, SIGNAL(valueChanged(double)), this, SLOT(OnSpecularChanged(double)));
   connect(mUI->lightSpecBlueEdit, SIGNAL(valueChanged(double)), this, SLOT(OnSpecularChanged(double)));
   connect(mUI->lightSpecPushbutton, SIGNAL(clicked()), this, SLOT(OnSpecularColorChooserClicked()));
}

//////////////////////////////////////////////////////////////////////////
dtInspectorQt::LightManager::~LightManager()
{
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OperateOn(dtCore::Base* b)
{
   dtCore::Light *light = dynamic_cast<dtCore::Light*>(b);

   mOperateOn = light;
   Update();
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OnEnabled(int checked)
{
   if (mOperateOn.valid())
   {
      mOperateOn->SetEnabled(checked ? true : false);
   }
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OnLightNumberChanged(int newValue)
{
   if (mOperateOn.valid())
   {
      mOperateOn->SetNumber(newValue);
   }
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OnModeChanged(const QString& newValue)
{
   if (mOperateOn.valid())
   {
      if (newValue == "GLOBAL")
      {
         mOperateOn->SetLightingMode(dtCore::Light::GLOBAL);
      }
      else if (newValue == "LOCAL")
      {
         mOperateOn->SetLightingMode(dtCore::Light::LOCAL);
      }
   }
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OnAmbientChanged(double newValue)
{
   if (mOperateOn.valid())
   {
      mOperateOn->SetAmbient(mUI->lightAmbRedEdit->value(),
         mUI->lightAmbGreenEdit->value(), mUI->lightAmbBlueEdit->value(), 1.0f);
   }
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OnAmbientColorChooserClicked()
{
   QColor initialColor(mUI->lightAmbRedEdit->value() * 255,
      mUI->lightAmbGreenEdit->value() * 255, mUI->lightAmbBlueEdit->value() * 255);
   QColor newColor = QColorDialog::getColor(initialColor, mUI->lightAmbPushbutton);
   if (newColor.isValid())
   {
      mUI->lightAmbRedEdit->setValue(newColor.redF());
      mUI->lightAmbGreenEdit->setValue(newColor.greenF());
      mUI->lightAmbBlueEdit->setValue(newColor.blueF());
   }
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OnDiffuseChanged(double newValue)
{
   if (mOperateOn.valid())
   {
      mOperateOn->SetDiffuse(mUI->lightDiffRedEdit->value(),
         mUI->lightDiffGreenEdit->value(), mUI->lightDiffBlueEdit->value(), 1.0f);
   }
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OnDiffuseColorChooserClicked()
{
   QColor initialColor(mUI->lightDiffRedEdit->value() * 255,
      mUI->lightDiffGreenEdit->value() * 255, mUI->lightDiffBlueEdit->value() * 255);
   QColor newColor = QColorDialog::getColor(initialColor, mUI->lightDiffPushbutton);
   if (newColor.isValid())
   {
      mUI->lightDiffRedEdit->setValue(newColor.redF());
      mUI->lightDiffGreenEdit->setValue(newColor.greenF());
      mUI->lightDiffBlueEdit->setValue(newColor.blueF());
   }
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OnSpecularChanged(double newValue)
{
   if (mOperateOn.valid())
   {
      mOperateOn->SetSpecular(mUI->lightSpecRedEdit->value(),
         mUI->lightSpecGreenEdit->value(), mUI->lightSpecBlueEdit->value(), 1.0f);
   }
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::OnSpecularColorChooserClicked()
{
   QColor initialColor(mUI->lightSpecRedEdit->value() * 255,
      mUI->lightSpecGreenEdit->value() * 255, mUI->lightSpecBlueEdit->value() * 255);
   QColor newColor = QColorDialog::getColor(initialColor, mUI->lightSpecPushbutton);
   if (newColor.isValid())
   {
      mUI->lightSpecRedEdit->setValue(newColor.redF());
      mUI->lightSpecGreenEdit->setValue(newColor.greenF());
      mUI->lightSpecBlueEdit->setValue(newColor.blueF());
   }
}

//////////////////////////////////////////////////////////////////////////
void dtInspectorQt::LightManager::Update()
{
   if (mOperateOn.valid())
   {
      mUI->lightGroupBox->show();

      mUI->lightEnabledToggle->setChecked(mOperateOn->GetEnabled());
      mUI->lightNumberEdit->setValue(mOperateOn->GetNumber());

      if (mOperateOn->GetLightingMode()==dtCore::Light::GLOBAL)
      {
         mUI->lightModeCombo->setCurrentIndex(mUI->lightModeCombo->findText("GLOBAL"));
      }
      else
      {
         mUI->lightModeCombo->setCurrentIndex(mUI->lightModeCombo->findText("LOCAL"));
      }

      float r,g,b,a;
      mOperateOn->GetAmbient(r, g, b, a);
      mUI->lightAmbRedEdit->setValue(r);
      mUI->lightAmbGreenEdit->setValue(g);
      mUI->lightAmbBlueEdit->setValue(b);

      mOperateOn->GetSpecular(r, g, b, a);
      mUI->lightDiffRedEdit->setValue(r);
      mUI->lightDiffGreenEdit->setValue(g);
      mUI->lightDiffBlueEdit->setValue(b);

      mOperateOn->GetDiffuse(r, g, b, a);
      mUI->lightSpecRedEdit->setValue(r);
      mUI->lightSpecGreenEdit->setValue(g);
      mUI->lightSpecBlueEdit->setValue(b);
   }
   else
   {
      mUI->lightGroupBox->hide();
   }
}

//////////////////////////////////////////////////////////////////////////

