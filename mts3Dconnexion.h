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

#ifndef _mts3Dconnexion_h
#define _mts3Dconnexion_h

#include <cisstMultiTask/mtsTaskPeriodic.h>

#include <cmath>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <conio.h>


#include <Windows.h>

#import "progid:TDxInput.Device.1" no_namespace

//Created by Marcin Balicki.

/*!
  \todo Remove all Windows.h and most system include from osa3Dconnexion.h (move to .cpp)
  \todo Add code to create as DLL on windows
  \todo Can we activate the buttons from code, i.e. not using external 3Dconnexion control panel
  \todo Use prm type for API?  At osa level, use vctTypes?
*/

//there exists an error if you install 3dx software on a different user.
//TODO : add calibrate/bias function
//TODO : add bypassing wizard settings. //looks like overall speed setting should be at max!
//TODO : test connection robustness.
//TODO : standardize values. (max is 1600 with full speed setting for both trans and rot)
//TODO : add button events
//TODO : check update rate seems sluggish with latency.
//TODO : remove the loop timer, use the automatic one.
//NOTE, use wizard to set the buttons to 1, and 2 (keystroke #s)

class mts3Dconnexion: public mtsTaskPeriodic {
    CMN_DECLARE_SERVICES(CMN_NO_DYNAMIC_CREATION, CMN_LOG_ALLOW_DEFAULT);

public:
    //This sets the name and calls Configure
    mts3Dconnexion(const std::string & taskName,
                   double period);

    //sets default "Dev1"
    ~mts3Dconnexion();

    void Startup(void);
    void Run(void);
    void Cleanup(void) {};

    //void SetDeviceName(std::string deviceName) {DeviceName=deviceName;};
    //Change this to suit your application
    //this sets the inputs and ouputs, please run this before using the other functions
    //possible to have tasks with a variety of arrangements
    //or multiple tasks. This should be done for convenience.
    //called by the constructor
    bool Configure(std::string configurationName = "");

    std::string GetConfigurationName(void) {
        return ConfigurationName;
    }

    void ReBias(void) {
        CMN_LOG_CLASS_INIT_ERROR << "Rebias not implemented yet" << std::endl;
    } // not implemented.

    //this masks the output of the device's 6 values, true is on. size 6.
    //0 for anything that has a false flag, by default all are open.
    //void SetAxisMask(const mtsBoolVec &mask);
    //void SetGain(const cmnDouble &gain);

protected:

    // this is the name used to loads the configuration settings from the 3dCon application
    std::string ConfigurationName;

    //state table elements
    //6 degree input from the 3d mouse.
    //it is displacement from 0 , proportional to force torque readings (but not precise
    mtsDoubleVec Pose;
    mtsBoolVec Buttons;

    // do this in the running thread otherwise it won't work...
    bool Init(void);

    // see setoutputmask.
    mtsBoolVec Mask;
    mtsDouble Gain;

    ISimpleDevicePtr _3DxDevice;
    ISensor * m_p3DSensor;
    IKeyboard * m_p3DKeyboard;
    ISimpleDevicePtr pSimpleDevice;
    MSG Msg;
    IVector3DPtr trans;
    IAngleAxisPtr rot;

    // time out for reading from usb.
    static const int timeout = 1; //seconds
};

CMN_DECLARE_SERVICES_INSTANTIATION(mts3Dconnexion);

#endif // _mts3Dconnexion_h