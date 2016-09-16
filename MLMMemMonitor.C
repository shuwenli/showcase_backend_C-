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
#include <math.h>

#include "MLMConstants.h"
#include "MLMMemMonitor.h"
#include "MLMConfiguration.h"
#include "MLMLogger.h"
#include "MLMProcess.h"
#include "MLMPIDScanner.h"
#include "MLMAlarm.h"

extern int g_MLMShutdown;

MLMMemMonitor * MLMMemMonitor::sm_instance = 0;

MLMMemMonitor::MLMMemMonitor() : m_enabled(true), m_freezeMode(false), m_firstScanFl(true), m_logOnlyMode(false), 
   m_initializationIntvl(InitializationIntvl_g), m_invocationIntvl(InvocationIntvl_g), 
   m_toleranceSlots(ToleranceSlotsMin_g), m_allowableTolCount(AllowableTolCount_g),
   m_numMonitoredProcesses(0), m_poolSz(MonitoredProcessesMax_g), 
   m_config(MLMConfiguration::Instance()), m_log(MLMLogger::Instance()) 
{
   for (unsigned short i=0; i<MonitoredProcessesMax_g; ++i) {
      m_availPool[MonitoredProcessesMax_g - i - 1] = i;
      m_monitoredProcesses[i].pid = 0;
      m_monitoredProcesses[i].poolidx = 0;
   }
   GetConfig(true, MaskBit_Initialization_Interval|MaskBit_Invocation_Interval|MaskBit_Tolerance_Interval|MaskBit_Enable_MLM|MaskBit_Allowable_Tolerance_Count);
   m_config.RegisterForConfigChanges(this);

   MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MEM, "Created MLMMemMonitor object of size %d bytes", sizeof(*this));
}

MLMMemMonitor::~MLMMemMonitor()
{
   MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MEM, "Destroyed MLMMemMonitor object of size %d bytes", sizeof(*this));

   delete MLMMemMonitor::sm_instance;
   MLMMemMonitor::sm_instance = 0;
}

MLMMemMonitor & MLMMemMonitor::Instance(void)
{
   if (MLMMemMonitor::sm_instance == 0) {
      MLMMemMonitor::sm_instance = new MLMMemMonitor;
   }
   return (*MLMMemMonitor::sm_instance);
}

short int MLMMemMonitor::GetConfig(bool isFirst, unsigned int chMask)
{
   if (chMask & MaskBit_Enable_MLM) {
      m_enabled = m_config.Enabled();
      if (m_config.m_invalidEnab[0] != 0) {
         MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Specified Enable_MLM %s is invalid", m_config.m_invalidEnab);
      }
      MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Enable_MLM is %d", m_enabled);
      if (!m_enabled) {
         DisableMLM();
      }
   }
   
   //dynamic updates to Initialization_Interval are not allowed
   if (isFirst && (chMask & MaskBit_Initialization_Interval)) {
      m_initializationIntvl = m_config.InitializationIntvl();
      if (m_config.m_invalidIniIntvl[0] != 0) {
         MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Specified Initialization_Interval %s is invalid", m_config.m_invalidIniIntvl);
      }
      MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Initialization_Interval is %u min", m_initializationIntvl);
      if (m_initializationIntvl == 0) {
         m_enabled = false;
         MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Changing Enable_MLM to %d", m_enabled);
      }
   }
      
   unsigned long oldInvIntvl = m_invocationIntvl;
   unsigned long oldTolSlots = m_toleranceSlots; 
   if (chMask & (MaskBit_Invocation_Interval | MaskBit_Tolerance_Interval)) {
      m_invocationIntvl = m_config.InvocationIntvl();
      if (m_config.m_invalidInvIntvl[0] != 0) {
         MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Specified Invocation_Interval %s is invalid", m_config.m_invalidInvIntvl);
      }
      MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Invocation_Interval is %u min", m_invocationIntvl);
   
      unsigned long tolIntvl = m_config.ToleranceIntvl();
      if (m_config.m_invalidTolIntvl[0] != 0) {
         MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Specified Tolerance_Interval %s is invalid", m_config.m_invalidTolIntvl);
      }
      MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Tolerance_Interval is %u min", tolIntvl);

      if (tolIntvl <= m_invocationIntvl) {
         MLM_LOG_DEBUG2(m_log, MLM_DEBUG_MIN, "Specified Tolerance_Interval=%u min is too short, for Invocation_Interval=%u min", tolIntvl, m_invocationIntvl);
         tolIntvl = m_invocationIntvl * ToleranceSlotsMin_g;
         MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Setting Tolerance_Interval to %u min", tolIntvl);
      }

      unsigned long tmpSlots = static_cast<unsigned long> (ceil(static_cast<long double>(tolIntvl/m_invocationIntvl)));
      if (tmpSlots < ToleranceSlotsMin_g) {
         MLM_LOG_DEBUG2(m_log, MLM_DEBUG_MIN, "The specified Tolerance_Interval=%u min is too short, for Invocation_Interval=%u min", tolIntvl, m_invocationIntvl);
         tolIntvl = m_invocationIntvl * ToleranceSlotsMin_g;
         tmpSlots = ToleranceSlotsMin_g;
         MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Setting Tolerance_Interval to %u min", tolIntvl);
      }

      if (tmpSlots > ToleranceSlotsMax_g) {
         MLM_LOG_DEBUG2(m_log, MLM_DEBUG_MIN, "The specified Tolerance_Interval=%u min is too long, for Invocation_Interval=%u min", tolIntvl, m_invocationIntvl)
         tolIntvl = m_invocationIntvl * ToleranceSlotsMax_g;
         tmpSlots = ToleranceSlotsMax_g;
         MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Setting Tolerance_Interval to %u min", tolIntvl);
      }

      m_toleranceSlots = tmpSlots;
      MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Number of Invocations per Tolerance_Interval is calculated as %u", m_toleranceSlots);

      AdjustProcessIntervals(oldInvIntvl, oldTolSlots);
   }

   //dynamic updates to Allowable_Tolerance_Count are not allowed
   if (isFirst && (chMask & MaskBit_Allowable_Tolerance_Count)) {
      m_allowableTolCount = m_config.AllowableTolCount();
      if (m_config.m_invalidATC[0] != 0) {
         MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Specified Allowable_Tolerance_Count %s is invalid", m_config.m_invalidATC);
      }
      MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Allowable_Tolerance_Count is %u", m_allowableTolCount);
   }
      
   if (isFirst && (chMask & MaskBit_Initialization_Interval) && (m_initializationIntvl == 0)) {
      MLM_LOG_DEBUG(m_log, MLM_DEBUG_MIN, "MLM is disabled permanently. To enable MLM, set Initialization_Interval to a non-zero value\n");
   }
   return (0);
}

void MLMMemMonitor::ConfigChanged(unsigned int chMask)
{
   if(chMask & (MaskBit_Invocation_Interval | MaskBit_Tolerance_Interval | MaskBit_Enable_MLM)) {
      GetConfig(false, chMask);
   }   
}

void MLMMemMonitor::AdjustProcessIntervals(unsigned long oldInvIntvl, unsigned long oldTolSlots)
{  
   MLMProcess * proc = 0;
   if (oldInvIntvl != m_invocationIntvl) {
      for (unsigned short i=0; i<m_numMonitoredProcesses; ++i) {
         proc = GetMonitoredProcessAt(i);
         proc->ResetToleranceInterval();
      }
   } else if (oldTolSlots != m_toleranceSlots) {
      for (unsigned short i=0; i<m_numMonitoredProcesses; ++i) {
         proc = GetMonitoredProcessAt(i);
         if (proc->GetCompletedToleranceSlots() >= m_toleranceSlots) {
            proc->ResetToleranceInterval();
         }
      }
   }
}

//returns 1 if the Idle Wait is interrupted by a SIGTERM signal 
unsigned short MLMMemMonitor::Idle(unsigned long min) const
{
   struct timeval tv;
   
   int rv = 0;
   int err = 0;
   while (true) {
      tv.tv_sec = min * 60;
      tv.tv_usec = 0;
      rv = select(0, 0, 0, 0, &tv);
      err = errno;
      MLM_LOG_DEBUG2(m_log, MLM_DEBUG_MAX, "select(), rv=%d, errno=%d", rv, err);

      if (rv == -1 && err == EINTR && g_MLMShutdown) return 1;
      if (rv == 0) break;
      else MLM_LOG_ERROR2(m_log, "error in select, rv=%d, errno=%d", rv, err);
   }
      
   return 0;
}

void MLMMemMonitor::WaitTillInitialization(bool enableLog /* = true */) const
{
   if (m_initializationIntvl == 0) return;

   if (enableLog) {
      MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MIN, "Initialization_Interval = %u min, waiting for Initialization Complete", m_initializationIntvl);
   }
   if (Idle(m_initializationIntvl) == 0 && enableLog ) { 
      MLM_LOG_DEBUG(m_log, MLM_DEBUG_MIN, "Initialization_Interval over");
   }
}

void MLMMemMonitor::WaitTillNextInvocation(void) const
{
   Idle(m_invocationIntvl);
}

void MLMMemMonitor::DisableMLM(void)
{
   for (unsigned short i=0; i<m_numMonitoredProcesses; ++i) {
         StopMonitoringProcessAt(i);
   }
}

//This method is called at every invocation interval
void MLMMemMonitor::UpdateProcesses(void)
{
   if (!m_enabled) return;

   MLMProcess * proc = 0;

   //Scan all currently running processes and record their current memory usage, create/add process objects to map for new processes
   MLMPIDScanner * pidScanner = new MLMPIDScanner;
   long pid = 0;

   for (pid = pidScanner->Next(); (pid != MLM_EOF && pid != MLM_ERROR); pid = pidScanner->Next(), proc = 0) {
      if (pid == 0) continue;

      proc = FindMonitoredProcess(pid);
      if (proc == 0) {
         //process is not being monitored currently, start monitoring this process,
         //provided we are not operating in freeze mode
         if (!m_freezeMode || m_firstScanFl) {
            proc = StartMonitoringForProcess(pid);
         }
      } 

      if (proc) {
         proc->RecordMemSize(m_logOnlyMode);
         proc->Touch();
      }
   }
   delete pidScanner;
   pidScanner = 0;

   unsigned short leaky = 0;
   proc = 0;

   m_log.StartLogUsage();
   bool haveLeakyProcess = false;
   for (unsigned short i=0; i<m_numMonitoredProcesses;) {
      proc = GetMonitoredProcessAt(i);
      
      if (proc->IsTouched() && !(proc->IsDead())) {
         proc->Untouch();

         // If this process has an alarm, ensure we indicate a leaky process
         haveLeakyProcess = haveLeakyProcess || proc->GetOutstandingAlarm();

         m_log.LogUsage(proc->GetTimestamp(), proc->GetPid(), proc->GetMemSize());

         if (proc->GetCompletedToleranceSlots() == m_toleranceSlots) {
            //check memory usage of the process if its Tolerance Interval has expired 
            leaky = proc->CheckMem();
            
            if (leaky >= 3) {
               if (proc->GetSuccessiveLeakyIntvls() >= m_allowableTolCount) {
                  if(!(proc->GetOutstandingAlarm())) proc->SetOutstandingAlarm();
                  proc->LogMemError(leaky);
                  haveLeakyProcess = true;
               } else {
                  proc->LogMemWarning(leaky);
               }
            } else {
               if (proc->GetOutstandingAlarm()) proc->ClearOutstandingAlarm();
            }
            proc->ResetToleranceInterval();
         }
         ++i;
         continue;
      } else {
         proc->SetDead();
         StopMonitoringProcessAt(i);
         continue;
      }
   }

   m_log.EndLogUsage();
   m_firstScanFl = false;
   // delete 8 lines below by shuwenli to support MSI
   //MLMAlarmIf& notifier = MLMAlarm::getAlarmAgent();
   //if (haveLeakyProcess) // Found one or more leaky processes?
   //{
    //  notifier.createAlarm(); // Raise an alarm
   //}
  // else
   //{
   //   notifier.clearAlarm(); // No leaky processes, so the alarm can be cleared
  // }
}

/*
   Obtains a reusable Process object, (attaches 'pid' to the Process object),
   and adds the associated pid/poolidx of the Process object to 'm_monitoredProcesses'
*/
MLMProcess * MLMMemMonitor::StartMonitoringForProcess(unsigned long pid)
{
   static int throttleLog = 0;
   short poolidx = GetReusableFromPool(pid);
   if (poolidx < 0) {
      //no more resources available in pool
      if (throttleLog == 0) {
         MLM_LOG_ERROR(m_log, "Cannot monitor any further processes because there are no more resources available in the pool");
         throttleLog = 1;
      }
      return (0);
   } 


   if (AddToMonitoredList(pid,poolidx) < 0) {
      MLM_LOG_ERROR1(m_log, "Attempt to monitor a process that was already monitored, pid=%u", pid);
      short dupMonIdx = FindInMonitoredList(pid);
      short dupPoolIdx = m_monitoredProcesses[dupMonIdx].poolidx;
      if (poolidx != dupPoolIdx) {
         RecycleToPool(poolidx);
      }
      return (&(m_pool[dupPoolIdx]));
   }

   return (&(m_pool[poolidx]));
}


void MLMMemMonitor::StopMonitoringProcessAt(unsigned short monidx)
{
   unsigned short poolidx = m_monitoredProcesses[monidx].poolidx;
   RecycleToPool(poolidx);
   DeleteFromMonitoredList(monidx);
   return;
}

/*
   If there are Process objects available in 'm_pool', obtains the next available
   object, updates 'm_availPool' and 'm_poolSz' and attaches the specified 'pid'
   to the Process object available from 'm_pool'. Returns the index of the Process object in 'm_pool'.

*/

short MLMMemMonitor::GetReusableFromPool(unsigned long pid)
{
   if (m_poolSz == 0) {
      return -1; //all resources from pool are depleted
   }

   m_poolSz--;
   short poolidx = m_availPool[m_poolSz];
   m_availPool[m_poolSz] = -1;
   m_pool[poolidx].AttachPid(pid);

   return (poolidx);
}

/*
   Takes as input 'poolidx' which is the index of the reusable Process object in 'm_pool'
   that is being recycled. Updates the status of available pool resources in 
   'm_availPool' and 'm_poolSz' and resets the recycled Process object to a 
   pid of '0'
 
*/
void MLMMemMonitor::RecycleToPool(unsigned short poolidx)
{
   if (m_poolSz == MonitoredProcessesMax_g) {
      m_poolSz = 0; //This is not needed but satisfies flexelint, since flexelint does
                    //not recognize the exit()
      //unexpected error, there cannot be more resources in the pool than the max size
      MLM_LOG_ERROR2(m_log, "Resource with poolidx=%u recycled to pool when pool size is already at max=%u\n", poolidx, MonitoredProcessesMax_g);
      exit(MLM_EXIT_CODE_UNEXPECTED);
   }

   m_poolSz++;
   m_availPool[m_poolSz - 1] = poolidx;
   m_pool[poolidx].AttachPid(0);

   return;
}

/*
   Add an element with pid=npid and idx=nidx to the list. If element already exists 
   returns -2, else return the position at which 
   it is inserted. Returns -1, and does not add the element, if maximun limit 
   is reached.
*/
short MLMMemMonitor::AddToMonitoredList(unsigned long npid, unsigned short nPoolIdx)
{
   if (m_numMonitoredProcesses == MonitoredProcessesMax_g) return (-1);

   short pos=0;
   bool insertBeforeFl = false;
   unsigned short i = 0;
   for (i=0; i<m_numMonitoredProcesses; i++) {
      if (m_monitoredProcesses[i].pid == npid) {
         //already exists in the monitored list
         pos = -2;
         break;
      } else if (m_monitoredProcesses[i].pid > npid) {
         insertBeforeFl = true;
         break;
      }
   }
   if (pos < 0) return (pos);

   if (i < m_numMonitoredProcesses) {
      for (unsigned short j=(m_numMonitoredProcesses-1); j>=i; j--) {
         m_monitoredProcesses[j+1].pid = m_monitoredProcesses[j].pid;
         m_monitoredProcesses[j+1].poolidx = m_monitoredProcesses[j].poolidx;
      }
   }
   m_monitoredProcesses[i].pid = npid;
   m_monitoredProcesses[i].poolidx = nPoolIdx;
   ++m_numMonitoredProcesses;
   return (i);
}

/*
   Deletes the element at 'monidx' position in 'm_monitoredProcesses'.
   If there are no elements in 'm_monitoredProcesses',
   returns -1. Else returns 0 for success.
*/
short MLMMemMonitor::DeleteFromMonitoredList(unsigned short monidx)
{
   if (m_numMonitoredProcesses == 0) return (-1);

   --m_numMonitoredProcesses;

   for (unsigned short j=monidx; j<m_numMonitoredProcesses; ++j) {
      m_monitoredProcesses[j].pid = m_monitoredProcesses[j+1].pid;
      m_monitoredProcesses[j].poolidx = m_monitoredProcesses[j+1].poolidx;
   }
   m_monitoredProcesses[m_numMonitoredProcesses].pid = 0;
   m_monitoredProcesses[m_numMonitoredProcesses].poolidx = 0;

   return (0);
}

/*
   Finds element with 'pid' that matches 'spid' in 'm_monitoredProcesses', and 
   returns its position in 'm_monitoredProcesses'. If there are no elements 
   in 'm_monitoredProcesses' or if the requested element cannot be found, 
   returns -1.
*/
short MLMMemMonitor::FindInMonitoredList(unsigned long spid)
{
   if (m_numMonitoredProcesses == 0) return (-1);

   if (m_numMonitoredProcesses == 1) {
      if (m_monitoredProcesses[0].pid == spid) return 0;
      else return -1;
   }

   unsigned short mid = m_numMonitoredProcesses/2;
   
   if (m_monitoredProcesses[mid].pid == spid) return (mid);
   if (m_monitoredProcesses[mid].pid < spid) return (FindInMonitoredList(spid, mid+1, m_numMonitoredProcesses-1));
   else return (FindInMonitoredList(spid, 0, mid-1));
   
}

/*
   overloaded method that does the same thing as FindInMonitoredList(spid), 
   except this one searches only within a given range [r1,r2]
*/
short MLMMemMonitor::FindInMonitoredList(unsigned long spid, unsigned short r1, unsigned short r2)
{
   if (r1 >= m_numMonitoredProcesses || r2 >= m_numMonitoredProcesses || r1 > r2) return (-1);

   if (r1 == r2) {
      if (m_monitoredProcesses[r1].pid == spid) return (r1);
      else return (-1);
   } 

   if (m_monitoredProcesses[r1].pid > spid || m_monitoredProcesses[r2].pid < spid) return (-1);

   unsigned short mid = (r2 - r1 + 1)/2 + r1;

   if (m_monitoredProcesses[mid].pid == spid) return (mid);
   if (m_monitoredProcesses[mid].pid < spid) return (FindInMonitoredList(spid, mid+1, r2));
   else return (FindInMonitoredList(spid, r1, mid-1));
}

/*
   Search 'm_monitoredProcesses' for element whose 'pid' matches 'spid'. If the
   element is not found returns a NULL pointer. If the element is found, determines
   the associated 'poolidx' of the 'Process' object in 'm_pool' and returns a pointer
   to the associated 'Process' object.
*/

MLMProcess * MLMMemMonitor::FindMonitoredProcess(unsigned long spid)
{
   MLMProcess * proc = 0;

   short monidx = FindInMonitoredList(spid);
   if (monidx >= 0) proc = &(m_pool[m_monitoredProcesses[monidx].poolidx]);

   return (proc);
}
