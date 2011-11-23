/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Marcin Balicki
  Created on: 2008-04-12

  (C) Copyright 2008-2011 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <saw3Dconnexion/mts3Dconnexion.h>
#include <cisstMultiTask/mtsInterfaceProvided.h>

CMN_IMPLEMENT_SERVICES(mts3Dconnexion);

mts3Dconnexion::mts3Dconnexion(const std::string & taskName,
                               double period):
    mtsTaskPeriodic(taskName, period, false, 1000)
{
    Pose.SetSize(6);
    Buttons.SetSize(2);

    Pose.SetAll(0);
    Buttons.SetAll(0);

    Mask.SetSize(6);
    Mask.SetAll(true);

    Gain = 1.0;

    //create interface to this task for major read write commands
    mtsInterfaceProvided * spaceNavInterface = AddInterfaceProvided("ProvidesSpaceNavigator");

    StateTable.AddData(Pose, "AxisData");
    StateTable.AddData(Buttons, "ButtonData");
    StateTable.AddData(Mask, "AxisMask");
    StateTable.AddData(Gain, "Gain");

    if (spaceNavInterface) {
        spaceNavInterface->AddCommandReadState(StateTable, Pose, "GetAxisData");
        spaceNavInterface->AddCommandReadState(StateTable, Buttons, "GetButtonData");
        spaceNavInterface->AddCommandReadState(StateTable, Mask, "GetAxisMask");
        spaceNavInterface->AddCommandWriteState(StateTable, Mask, "SetAxisMask");
        spaceNavInterface->AddCommandReadState(StateTable, Gain, "GetGain");
        spaceNavInterface->AddCommandWriteState(StateTable, Gain, "SetGain");
        spaceNavInterface->AddCommandVoid(&mts3Dconnexion::ReBias, this, "ReBias");
    }
}


bool mts3Dconnexion::Configure(std::string configurationName)
{
    ConfigurationName = configurationName;
    return true;
}


bool mts3Dconnexion::Init(void)
{
    //call this in the thread.
    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (!SUCCEEDED(hr)) {
        std::cout<<" could not initialize "<<std::endl;
        return false;
    }

    hr = _3DxDevice.CreateInstance(__uuidof(Device));
    //std::cout<<hr<<std::endl;
    //Sleep(3000);

    if (SUCCEEDED(hr)) {
        pSimpleDevice = _3DxDevice;
        if (pSimpleDevice != NULL) {
            long type;
#define UnknownDevice 0
            hr = _3DxDevice->QueryInterface(&pSimpleDevice);
            hr = pSimpleDevice->get_Type(&type);
            if (type == UnknownDevice) {
                CMN_LOG_CLASS_INIT_ERROR << "Space Navigator not found!" << std::endl;
                //is driver running, is the device plugged in?
                return false;
                //osaSleep(3000);
                //exit(-1);
            }
            if (SUCCEEDED(hr) && type != UnknownDevice) {

                // Get the interfaces to the sensor and the keyboard;
                m_p3DSensor = pSimpleDevice->Sensor;
                m_p3DKeyboard = pSimpleDevice->Keyboard;

                pSimpleDevice->LoadPreferences(ConfigurationName.c_str());
                // Connect to the driver
                hr = pSimpleDevice->Connect();  ///this returns no matter if the device is connected or not.
                //std::cout<<hr<<std::endl;
                CMN_LOG_CLASS_INIT_VERBOSE << "SpaceNavigator connected..." << std::endl;
                return true;

            }
            else {
                std::cout << " could not initialize "<< std::endl;
                return false;
            }
        }
    }
    return true;
}


mts3Dconnexion::~mts3Dconnexion()
{
    // Release the sensor and keyboard interfaces
    if (m_p3DSensor) {
        m_p3DSensor->get_Device((IDispatch**)&_3DxDevice);
        m_p3DSensor->Release();
    }

    if (m_p3DKeyboard) {
        m_p3DKeyboard->Release();
    }

    if (_3DxDevice) {
        // Disconnect it from the driver
        _3DxDevice->Disconnect();
        _3DxDevice->Release();
    }
    //should remove isntances of the space mouse
}


void mts3Dconnexion::Startup(void)
{
}


void mts3Dconnexion::Run(void)
{
    ProcessQueuedCommands();

    //note from the web:
    //I already found out that I have to call CoInitalize(NULL) in the thread and the thread must
    //live until the main thread ends. Otherwise I would get marshalling errors.
    static bool hasInit = false;

    //The 3D mouse is a little tricky and has to be initialized in this thread
    //after the thread has been started ( done internally in runOnce()
    //The initialization might take 500ms

    if (!hasInit) {
        hasInit = Init();//This call takes about .5 ms.
        return;
    }

    if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    //if its in the update loop of the mouse and you will handle the message outside
    //simply use PM_NOREMOVE and call GetMessage again outside...

    if (m_p3DSensor) {
        try {
            trans = m_p3DSensor->GetTranslation();
            rot = m_p3DSensor->GetRotation();
            Pose[0] = trans->GetX();
            Pose[1] = trans->GetY();
            Pose[2] = trans->GetZ();
            //AngleAxis component interface
            //The  AngleAxis  object  provides  a  representation  for  orientation  in  3D  space  using  an
            //angle and an axis. The rotation is specified by a normalized vector and an angle around
            //the vector. The rotation is the right-hand rule.
            double angle;
            rot->get_Angle(&angle); //intensity so multiply by normalized angle.
            Pose[3] = rot->GetX() * angle;
            Pose[4] = rot->GetY() * angle;
            Pose[5] = rot->GetZ() * angle;
            //now lets add a gain.
            Pose *= Gain.Data;

            //filter it
            for (unsigned int i = 0; i < 6; i++) {
                if (Mask[i] == false) {
                    Pose[i] = 0.0;
                }
            }

            //vctAxAnRot3 axisAngle(Pose.Data[3],Pose.Data[4],Pose.Data[5]);
            //convert to euler angle for readability...
        }
        catch (...) {
            CMN_LOG_CLASS_RUN_ERROR << "EXCEPTION!" << std::endl;
        }
    }

    /* 0 == FALSE, -1 == TRUE */
    VARIANT_BOOL res;
    if (m_p3DKeyboard) {

        try {
            //m_p3DKeyboard->GetKeys();
            long b = m_p3DKeyboard->GetKeys();
            //std::cout<<"Num of keys: "<<b<<std::endl;
            // std::cout<<"Key names: "<<m_p3DKeyboard->GetKeyName(30)<<" "<<m_p3DKeyboard->GetKeyName(30)<<std::endl;
            //special keys
            //this really depends on the setup in the wizzard..
            //fit is 31, wizard is 32
            //res=m_p3DKeyboard->IsKeyDown(32); //this seems to not be a possiblity...throws exception
            //std::cout<<std::endl;
            //__int64 mask = (__int64)1<<(i-1);

            //NOTE: setup the keys in the way so you can chooze these numbers!
            //left button 1, right button 2!!!
            res = m_p3DKeyboard->IsKeyDown(1); //this seems to not be a possiblity...throws exception

            if (res==VARIANT_TRUE) {
                Buttons[0] = true;
            } else {
                Buttons[0] = false;
            }

            res = m_p3DKeyboard->IsKeyDown(2);
            if (res == VARIANT_TRUE) {
                Buttons[1] = true;
            } else {
                Buttons[1] = false;
            }
        }
        catch (...) {
            CMN_LOG_CLASS_RUN_ERROR << "EXCEPTION!" << std::endl;
        }
    }
}
