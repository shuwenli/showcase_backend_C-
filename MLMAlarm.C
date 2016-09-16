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

#include "MLMAlarm.h"
#include "MLMDynLib.h"
#include "MLMConstants.h"
#include "MLMMemMonitor.h" // Memory monitor class definition
#include "MLMLogger.h" // Logger class definition

/**
 * Get the alarm agent.
 *
 * Uses the MLMDynLib class to load the libmlmfoam.so library to obtain an
 * instance of the MLMAlarmIf interface for handling MLM alarms.  If the
 * library can't be loaded or if the method name can't be found, this method
 * returns a singleton instance of the MLMAlarm class.  This is an
 * adaptation of the strategy pattern where the object returned is conditional
 * on the services that are available.
 *
 * @return MLMAlarmIf The agent that implements the alarm interface.
 */
MLMAlarmIf&
MLMAlarm::getAlarmAgent()
{
    static MLMAlarmIf* pAlarmIf = 0; // Pointer to the alarm interface object

    // See if this is the first call to this method.
    if (pAlarmIf == 0 &&
        !MLMMemMonitor::Instance().IsLogOnlyMode()) // Not just logging?
    {
        try
        {
            // The MLMDynLib object is defined statically so that it persists
            // beyond the scope of this method call.  The reason being that the
            // library needs to remain open while it is being used.
            static MLMDynLib dynLib(MLMConstants::LibraryName_g);

            // Look-up the alarm interface method from the library
            const char * const pGetMLMAlarmIf = "getMLMAlarmIf";
            void* pAlarmIfAddr = dynLib.getAddress(pGetMLMAlarmIf);

            if (pAlarmIfAddr != 0)   // Have a valid address?
            {
                /*
                 * Define the dynamic function prototype and copy the address
                 * to it
                 */
                MLMAlarmIf& (*pGetAlarmIfFunc)() =
                    reinterpret_cast<MLMAlarmIf& (*)()>(pAlarmIfAddr);
                // Dynamically call the function to get the interface object:
                pAlarmIf = &(*pGetAlarmIfFunc)();
            }
            else // Can't find the method
            {
                std::string what("No alarms will be generated.  "
                    "Cannot find \"");
                what += pGetMLMAlarmIf;
                what += "\" in \"";
                what += MLMConstants::LibraryName_g;
                what += "\"";
                MLMLogger::Instance().LogError(what.c_str());
            }
        }
        catch (const std::exception& ex)
        {
            std::string what("No alarms will be generated.  ");
            what += ex.what();
            MLMLogger::Instance().LogError(what.c_str());
        }
    }

    if (pAlarmIf == 0) // Unable to get the alarm agent from the library?
    {
        // Use a no-op instance of the MLMAlarm class
        static MLMAlarm theInstance;
        pAlarmIf = &theInstance;
    }

    return *pAlarmIf; // Use the alarm agent provided by the library
}
