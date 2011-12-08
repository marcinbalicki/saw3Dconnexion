/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
Author(s):	Marcin Balicki
Created on:   2008-08-12

(C) Copyright 2008 Johns Hopkins University (JHU), All Rights
Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include "UITask.h"
#include <cisstMultiTask/mtsInterfaceRequired.h>

CMN_IMPLEMENT_SERVICES(UITask);

UITask::UITask(const std::string & taskName,
               double period):
    mtsTaskPeriodic(taskName, period, false, 5000),
    ExitFlag(false)
{
    AnalogInput.SetSize(6);
    DigitalInput.SetSize(2);

    AnalogInput.SetAll(0);
    DigitalInput.SetAll(true);

    //open a resource port that will connect to the device's provided interface.

    mtsInterfaceRequired * SNInterface = this->AddInterfaceRequired("RequiresSpaceNavigator");

    if (SNInterface) {
        SNInterface->AddFunction( "GetButtonData", GetDigitalInput);
        SNInterface->AddFunction( "GetAxisData", GetAnalogInput);
    } else {
        CMN_LOG_CLASS_INIT_ERROR << "Startup: failed to find required interfaces" << std::endl;
    }
}


void UITask::Startup(void)
{
    Mouse3DGUI.DisplayWindow->show();
}


void UITask::Run(void)
{
    Fl::check();
    if (Fl::thread_message() != 0) {
        //gui error
        CMN_LOG_CLASS_RUN_ERROR << "GUI Error" << Fl::thread_message() << std::endl;
        return;
    }

    //ProcessQueuedEvents();
    //else do lots of gui updating
    if (Mouse3DGUI.DoClose == true) {
        //exit flag for the main thread (higher)
        ExitFlag = true;
    }

    GetDigitalInput(DigitalInput);
    GetAnalogInput(AnalogInput );
    for (unsigned int i = 0; i < 6; i++) {
        Mouse3DGUI.AnalogInput[i]->value(AnalogInput[i]);
    }
    
    Mouse3DGUI.DigitaInput[0]->value(!DigitalInput[0]);
    Mouse3DGUI.DigitaInput[1]->value(!DigitalInput[1]);
}
