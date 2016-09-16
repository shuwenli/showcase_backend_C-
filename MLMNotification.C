/* -*- c++ -*- */ /* vim:set ts=4 sts=4 sw=4 et syntax=cpp: */
/************************************************************************/
/*                                                                      */
/*  COPYRIGHT (c) 2006 Lucent Technologies                              */
/*  All Rights Reserved.                                                */
/*                                                                      */
/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF LUCENT TECHNOLOGIES  */
/*                                                                      */
/*  The copyright notice above does not evidence any                    */
/*  actual or intended publication of such source code.                 */
/*                                                                      */
/************************************************************************/

#include <string>
#include <exception>

#include "MLMNotification.h"
#include "MLMDynLib.h"
#include "MLMConstants.h"
#include "MLMMemMonitor.h" // Memory monitor class definition
#include "MLMLogger.h" // Logger class definition

/**
 * Get the notification agent.
 *
 * Uses the MLMDynLib class to load the libmlmfoam.so library to obtain an
 * instance of the MLMNotificationIf interface for handling MLM notifications.
 * If the library can't be loaded or if the method name can't be found, this
 * method returns a singleton instance of the MLMNotification class.  This is an
 * adaptation of the strategy pattern where the object returned is conditional
 * on the services that are available.
 *
 * @return MLMNotificationIf The agent that implements the notification
 * interface.
 */
MLMNotificationIf&
MLMNotification::getNotificationAgent()
{
    // Pointer to the notification interface object
    static MLMNotificationIf* pNotifIf = 0;

    // See if this is the first call to this method.
    if (pNotifIf == 0 &&
        !MLMMemMonitor::Instance().IsLogOnlyMode()) // Not just logging?
    {
        try
        {
            // The MLMDynLib object is defined statically so that it persists
            // beyond the scope of this method call.  The reason being that the
            // library needs to remain open while it is being used.
            static MLMDynLib dynLib(MLMConstants::LibraryName_g);

            // Look-up the notification interface method from the library
            const char * const pGetMLMNotificationIf = "getMLMNotificationIf";
            void* pNotifIfAddr = dynLib.getAddress(pGetMLMNotificationIf);

            if (pNotifIfAddr != 0) // Have a valid address?
            {
                /*
                 * Define the dynamic function prototype and copy the address
                 * to it
                 */
                MLMNotificationIf & (*pGetNotifIfFunc)() =
                    reinterpret_cast<MLMNotificationIf & (*)()>(pNotifIfAddr);
                // Dynamically call the function to get the interface object:
                pNotifIf = &(*pGetNotifIfFunc)();
            }
            else // Can't find the method
            {
                std::string what("No notifications will be generated.  "
                    "Cannot find \"");
                what += pGetMLMNotificationIf;
                what += "\" in \"";
                what += MLMConstants::LibraryName_g;
                what += "\"";
                MLMLogger::Instance().LogError(what.c_str());
            }
        }
        catch (const std::exception& ex)
        {
            std::string what("No notifications will be generated.  ");
            what += ex.what();
            MLMLogger::Instance().LogError(what.c_str());
        }
    }

    if (pNotifIf == 0) // Unable to get the notification agent from the library?
    {
        // Use a no-op instance of the MLMNotification class
        static MLMNotification theInstance;
        pNotifIf = &theInstance;
    }

    return *pNotifIf; // Use the notification agent provided by the library
}
