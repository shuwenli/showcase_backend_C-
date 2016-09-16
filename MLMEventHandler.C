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
#include "event/EVapi.h" // FOAM Event Handler API

#include "MLMLogger.h" // Logger class definition
#include "MLMEventHandler.h" // Class definition

extern "C"
{
    /**
     * Get an instance of the MLM alarm interface.  Returns a reference
     * to the singleton instance of the MLMEventHandler class.
     *
     * @return a reference to an instance of the MLM Alarm interface.
     */
    MLMAlarmIf& getMLMAlarmIf()
    {
        return MLMEventHandler::getInstance();
    }

    /**
     * Get an instance of the MLM notification interface.  Returns a reference
     * to the singleton instance of the MLMEventHandler class.
     *
     * @return a reference to an instance of the MLM Notification interface.
     */
    MLMNotificationIf& getMLMNotificationIf()
    {
        return MLMEventHandler::getInstance();
    }
}

/**
 * Ctor.
 * Initializes the MO ID array member.
 */
MLMEventHandler::MLMEventHandler()
{
    for (int i=0; i<sizeof(m_MoIds) / sizeof(m_MoIds[0]); i++)
    {
        m_MoIds[i] = 0;
    }
    EVGETLOCALNODE(m_MoIds[0]); // Populate the MO ID of the local node
}

/**
 * Get the singleton instance.
 *
 * @return MLMEventHandler
 */
MLMEventHandler&
MLMEventHandler::getInstance()
{
    static MLMEventHandler theInstance;
    return theInstance;
}

/**
 * Log an error entry.
 *
 * @param what string identifying what was tried.
 * @param errCode error code indicating what went wrong.
 */
static void
logError(const char* const what, int errCode)
{
    /*
     * An ostringstream is the C++ way to go, but the platform is stuck between
     * "classic" and std iostreams forever, so the old "C" way will have to do.
     */
    char errBuff[512];
    snprintf(errBuff, sizeof(errBuff), "%s returned %d", what, errCode);
    MLMLogger::Instance().LogError(errBuff);
}

/**
 * Create a new alarm.  Uses the EV_REPORT_ALARM API of the FOAM event handler
 * to create an alarm with the FLXEV_MLM_POTENTIAL_LEAK key.  The FOAM ecf
 * file defines the text of the alarm.
 *
 * @param MLMAlarmIfIndex (UNUSED) index of the unique key identifying the
 * alarm.
 * @param string (UNUSED) String providing the alarm description.
 */
void
MLMEventHandler::createAlarm(MLMAlarmIfIndex, const std::string &)
{
    int evRet = EV_REPORT_ALARM(EVMO_AP, m_MoIds, FLXEV_MLM_POTENTIAL_LEAK, 0);

    /*
     * Check if the call failed and is due to any reason other than the
     * alarm already existing.
     */
    if (evRet != EV_SUCCESS && evRet != EV_EEXIST)
    {
        logError("EV_REPORT_ALARM(FLXEV_MLM_POTENTIAL_LEAK)", evRet);
    }
}

/**
 * Clear an existing alarm.  Uses the EV_REPORT_ALARM_CLEAR API of the FOAM
 * event handler to clear an alarm with the FLXEV_MLM_POTENTIAL_LEAK key.
 *
 * @param MLMAlarmIfIndex (UNUSED) index of the unique key identifying the
 * alarm.
 */
void
MLMEventHandler::clearAlarm(MLMAlarmIfIndex)
{
    int evRet = EV_REPORT_ALARM_CLEAR(EVMO_AP, m_MoIds,
        FLXEV_MLM_POTENTIAL_LEAK);

    /*
     * Check if the call failed and is due to any reason other than the
     * alarm not existing.
     */
    if (evRet != EV_SUCCESS && evRet != EV_NO_ENTRY)
    {
        logError("EV_REPORT_ALARM_CLEAR(FLXEV_MLM_POTENTIAL_LEAK)", evRet);
    }
}

/**
 * Send a notification.  Uses the EV_REPORT_REPTONLY API of the FOAM
 * event handler to send a notification with the
 * FLXEV_MLM_POTENTIAL_LEAK_PROCESS key.
 *
 * @param pID Process ID.
 * @param pName Process name.
 * @param memUsage Amount of memory used in bytes.
 * @param tolerance String providing the tolerance interval.
 */
void
MLMEventHandler::sendNotification(unsigned long pID, const std::string& pName,
    unsigned long memUsage, const std::string& tolerance)
{
    int evRet = EV_REPORT_REPTONLY(EVMO_AP, m_MoIds,
        FLXEV_MLM_POTENTIAL_LEAK_PROCESS, 4, pName.c_str(), pID, memUsage,
        tolerance.c_str());

    if (evRet != EV_SUCCESS) // Check if the call failed
    {
        logError("EV_REPORT_REPTONLY(FLXEV_MLM_POTENTIAL_LEAK_PROCESS)", evRet);
    }
}
