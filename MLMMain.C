/* -*- c++ -*- */ /* vim:set ts=3 sts=3 sw=3 et syntax=cpp: */
/************************************************************************/
/*                                                                      */
/*  COPYRIGHT (c) 2007 Alcatel Lucent                                   */
/*  All Rights Reserved.                                                */
/*                                                                      */
/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF ALCATEL LUCENT       */
/*                                                                      */
/*  The copyright notice above does not evidence any                    */
/*  actual or intended publication of such source code.                 */
/*                                                                      */
/************************************************************************/
#include <stdlib.h>
#include <signal.h>
#include "MLMConstants.h"
#include "MLMMemMonitor.h"
#include "MLMConfiguration.h"
#include "MLMLogger.h"
#include "MLMAlarm.h"

// delete 3 lines by shuwenli to support MSI
//#include "rcc/errmsg.h"
//#include "rcc/ssapi.h"
//#include "rcc/rccadm_api.h"
//============================================================================================
// MAIN PROGRAM
//============================================================================================

int g_MLMShutdown = 0;

extern "C" 
{
   void MLMSignalHandler(int sig)
   {
      if (sig == SIGTERM) g_MLMShutdown = 1;
   }
}

int main(int argc, char * argv[])
{
   extern int opterr;
   extern char *optarg;
   opterr = 1;

   //Initialize global variables
   g_MLMShutdown = 0;
   
   //Initialize RCC Error Message Library
   //delete 3 line below by shuwenli to support MSI
//   char rccProc[SSNAMESIZE];
//   sprintf(rccProc, "APPLPROC-%ld", (long)getpid());
//   EMSGINIT(rccProc, EMPC_APPLPROC, EML_INFO);

   //Setup Signal Handler
   struct sigaction sigCfg;
   sigemptyset(&sigCfg.sa_mask);
   sigaddset(&sigCfg.sa_mask, SIGTERM);
   sigCfg.sa_flags = 0;
   sigCfg.sa_handler = MLMSignalHandler;
   if (sigaction(SIGTERM, &sigCfg, 0) < 0) {
//  delete 1 line below by shuwenli to support MSI
//      ERRMSG(EML_ERROR, EMS_MINOR, "Cannot setup SIGTERM signal handler", 0, 2, errno, SS_vmName());
   }

   //Initalize
   MLMConfiguration & config = MLMConfiguration::Instance();
   MLMLogger & log = MLMLogger::Instance();
   MLMMemMonitor & mlm = MLMMemMonitor::Instance();

   int rv = 0;
   while ((rv = getopt(argc, argv, "fl")) != EOF)
   {
      switch(rv) {
      case 'f':
         MLM_LOG_DEBUG(log, MLM_DEBUG_4, "MLM will run in freeze mode");
         mlm.SetFreezeMode();
         break;
      case 'l':
         MLM_LOG_DEBUG(log, MLM_DEBUG_4, "MLM will run in log-only mode");
         mlm.SetLogOnlyMode();
         break;
      default :
         MLM_LOG_ERROR2(log, "Use of undefined option on command-line: %c, aborting with exit code %d\n", rv, MLM_EXIT_CODE_UNDEFINED_OPTION);
         exit(MLM_EXIT_CODE_UNDEFINED_OPTION);
      };
   }

   //Report to PMON that MLM has initialized
   //delete 4 lines below by shuwenli to supporty MSI
   //int rccRV = 0;
   //if ((rccRV = RCA_pmprocinited(SS_vmName(), argv[0], -1)) != 0) {
   //   ERRMSG(EML_ERROR, EMS_MAJOR, "RCA_pmprocinited failed", 0, 6, rccRV, SS_vmName());
   //} 
   
   // Clear any existing MLM alarms
   //delete 1 line below by shuwenli to support MSI
   //MLMAlarm::getAlarmAgent().clearAlarm();

   bool MLMEnabled = false;
   bool firstTime = true;
   while (!MLMEnabled) {
      //Wait till the initialization interval is over and MLM is enabled
      mlm.WaitTillInitialization(firstTime);

      if (g_MLMShutdown) {
         MLM_LOG_DEBUG(log, MLM_DEBUG_MIN, "Received SIGTERM, exiting\n");
         exit(MLM_SUCCESS);
      }

      config.Read();
      MLMEnabled = config.Enabled();
      if (!MLMEnabled && firstTime) {
         MLM_LOG_DEBUG(log, MLM_DEBUG_4, "MLM is disabled. Waiting ...");
      }
      firstTime = false;
   }

   if (g_MLMShutdown) {
      MLM_LOG_DEBUG(log, MLM_DEBUG_MIN, "Received SIGTERM, exiting\n");
      exit(MLM_SUCCESS);
   }

   //Lock configuration data, updates to meta-dynamic config parameters is no longer allowed
   log.LockConfig();

   MLM_LOG_DEBUG(log, MLM_DEBUG_4, "Starting first invocation of MLM ...");

   while (true) {
      mlm.UpdateProcesses();

      //sleep until the next Invocation Interval
      mlm.WaitTillNextInvocation();

      if (g_MLMShutdown) {
         MLM_LOG_DEBUG(log, MLM_DEBUG_MIN, "Received SIGTERM, exiting\n");
         exit(MLM_SUCCESS);
      } 

      config.Read();
   }
   
   return (0);
}
