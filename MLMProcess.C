nn/* -*- c++ -*- */ /* vim:set ts=3 sts=3 sw=3 et syntax=cpp: */
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <procfs.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#include "MLMConfiguration.h"
#include "MLMConstants.h"
#include "MLMProcess.h"
#include "MLMLogger.h"
#include "MLMAlarm.h"
#include "MLMNotification.h"

MLMProcess::MLMProcess(void) : 
   m_pid(0), 
   m_currPercentMem(0), 
   m_completedToleranceSlots(0), 
   m_memSizeDataSz(0), 
   m_memUsageAvg(0), 
   m_memUsageAvgPrev(0), 
   m_isLeaky(0),
   m_isLeakyPrev(0),
   m_isOutstandingAlarm(false),
   m_touchFl(true), 
   m_isDeadFl(true), 
   m_timestamp(0),
   m_successiveLeakyIntvls(0), 
   m_log(MLMLogger::Instance())
{
   m_psinfoFileName[0] = 0;
   for (int i=0; i<ToleranceSlotsMax_g; ++i) m_memSizeData[i] = 0;
   MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MEM, "Created MLMProcess object of size %d bytes", sizeof(*this));
}

MLMProcess::MLMProcess(const MLMProcess & rhs) : 
   m_pid(rhs.m_pid), 
   m_currPercentMem(rhs.m_currPercentMem), 
   m_completedToleranceSlots(rhs.m_completedToleranceSlots), 
   m_memSizeDataSz(rhs.m_memSizeDataSz),
   m_memUsageAvg(rhs.m_memUsageAvg),
   m_memUsageAvgPrev(rhs.m_memUsageAvgPrev),
   m_isLeaky(rhs.m_isLeaky),
   m_isLeakyPrev(rhs.m_isLeakyPrev),
   m_isOutstandingAlarm(rhs.m_isOutstandingAlarm),
   m_touchFl(rhs.m_touchFl),
   m_isDeadFl(rhs.m_isDeadFl),
   m_timestamp(0),
   m_successiveLeakyIntvls(rhs.m_successiveLeakyIntvls), 
   m_log(MLMLogger::Instance())
{
   strcpy(m_psinfoFileName, rhs.m_psinfoFileName);
   for (int i=0; i<ToleranceSlotsMax_g; ++i) m_memSizeData[i] = rhs.m_memSizeData[i];
   MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MEM, "Created copy MLMProcess object of size %d bytes", sizeof(*this));
}

void MLMProcess::AttachPid(unsigned long pid)
{
   if (!m_isDeadFl) {
      MLM_LOG_DEBUG2(m_log, MLM_DEBUG_MAX, "Attaching PID=%u to a non-dead process object with pid=%u", pid, m_pid);
   }

   m_pid = pid;
   m_currPercentMem = 0;
   m_completedToleranceSlots = 0;
   m_memSizeDataSz = 0;
   m_memUsageAvg = 0;
   m_memUsageAvgPrev = 0;
   m_touchFl = true;
   m_psinfoFileName[0] = 0;
   m_timestamp = 0;
   m_isOutstandingAlarm = false;

   if (pid == 0) {
      m_isDeadFl = true;
      return;
   }

   m_isDeadFl = false;
   sprintf(m_psinfoFileName, "%s/%u/psinfo", MLMConstants::ProcDir_g, m_pid);
   for (int i=0; i<ToleranceSlotsMax_g; ++i) m_memSizeData[i] = 0;
   LogMapProcessData();

   return;
}

MLMProcess::~MLMProcess()
{
   MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MEM, "Destroyed MLMProcess object of size %d bytes", sizeof(*this));

   if (m_pid) {
      sprintf(MLMLogger::sm_buffer, "Deleted MLMProcess: pid=%u", m_pid);
      m_log.LogMap(MLMLogger::sm_buffer);
   }
}

unsigned long MLMProcess::ComputeCurrentMemSize(time_t* p_timestamp)
{
   if (m_isDeadFl) return (0);

   int fd = 0;
   if (p_timestamp) {
      time(p_timestamp);
   }
   if ((fd = open(m_psinfoFileName, O_RDONLY)) == -1) {
      //MLMProcess does not exist anymore
      MLM_LOG_DEBUG1(m_log, MLM_DEBUG_4, "MLMProcess with pid=%u is no longer alive", m_pid);
      m_isDeadFl = true;
      return (0);
   } 

   psinfo_t info;
   int saveErrno = 0;
   while (read(fd, &info, sizeof(psinfo_t)) < 0 ) {
      saveErrno = errno;
      close(fd);
      if( saveErrno == EAGAIN ) continue;
      if( saveErrno != ENOENT ) return (0);
   }
   close(fd);
   m_currPercentMem = info.pr_pctmem;
   return (info.pr_size);
}

void MLMProcess::RecordMemSize(bool logOnly /*= false */)
{
   if (m_isDeadFl) return;

   unsigned long sz = ComputeCurrentMemSize(&m_timestamp);
   if (logOnly) {
      m_memSizeData[0] = sz;
      m_memSizeDataSz = 1;
   } else {
      m_memSizeData[m_memSizeDataSz++] = sz;
      ++m_completedToleranceSlots;
   }
   MLM_LOG_DEBUG2(m_log, MLM_DEBUG_MAX, "Mem Usage: pid=%u, mem=%u", m_pid, sz);

   if (m_memSizeDataSz > ToleranceSlotsMax_g) {
      m_memSizeDataSz = 0; //This is not really needed, but satisfies flexelint
                           //since flexelint does not recognize the exit()
      MLM_LOG_ERROR2(m_log, "Memory-usage data space exhausted for process with pid=%u, aborting with exit code=%d\n", m_pid, MLM_EXIT_CODE_UNEXPECTED);
      exit(MLM_EXIT_CODE_UNEXPECTED);
   }
   return;
}

void MLMProcess::LogMapProcessData(void)
{
   if (m_isDeadFl) return;

   int fd = 0;
   if ((fd = open(m_psinfoFileName, O_RDONLY)) == -1) {
      //MLMProcess does not exist anymore
      MLM_LOG_DEBUG1(m_log, MLM_DEBUG_4, "MLMProcess with pid=%u is no longer alive", m_pid);
      m_isDeadFl = true;
      return;
   } 

   psinfo_t info;
   int saveErrno = 0;
   while (read(fd, &info, sizeof(psinfo_t)) < 0 ) {
      saveErrno = errno;
      close(fd);
      if( saveErrno == EAGAIN ) continue;
      if( saveErrno != ENOENT ) return;
   }
   close(fd);

   sprintf(MLMLogger::sm_buffer, "MLMProcess: pid=%u, parent=%u, name=%s, cmd=%s", info.pr_pid, info.pr_ppid, info.pr_fname, info.pr_psargs);
   m_log.LogMap(MLMLogger::sm_buffer);
   m_name = info.pr_fname;
   return;
}

unsigned short MLMProcess::CheckMem(void)
{
   m_isLeaky = 0;

   m_memUsageAvg = m_memSizeData[0];

   //check for progressive increase in memory usage over the current tolerance interval
   bool isDidDecrease = false;
   bool isDidIncrease = false;
   for (unsigned int i=1; i<m_memSizeDataSz; ++i) {
      if (m_memSizeData[i] < m_memSizeData[i-1]) isDidDecrease = true;
      if (m_memSizeData[i] > m_memSizeData[i-1]) isDidIncrease = true;
      m_memUsageAvg +=  m_memSizeData[i];
   }
   if (isDidIncrease && !isDidDecrease) m_isLeaky = 1;

   //calculate the average memory usage for the current tolerance interval
   if (m_memSizeDataSz) m_memUsageAvg /= m_memSizeDataSz;
   else {
      MLM_LOG_ERROR1(m_log, "MLMProcess::CheckMem called when Memory-usage data size is 0, exiting with error code %d\n", MLM_EXIT_CODE_UNEXPECTED);
      exit(MLM_EXIT_CODE_UNEXPECTED);
   }

   //compare current average with previous average
   //if previous average was zero, shouldn't compare averages
   if (m_memUsageAvgPrev) {
      if (m_memUsageAvg > m_memUsageAvgPrev) {
         m_isLeaky = 2;
      }
   }

   if (m_isLeaky || (m_memUsageAvgPrev == 0)) {
      double r = 0.0;
      //calculate correlation coefficient
      r = CalculateTrendFactor();
      if (r >= 0.7) { 
         m_isLeaky = 3;
      }
   }
   
   if (m_isLeakyPrev == 3 && m_isLeaky == 3) ++m_successiveLeakyIntvls;
   else if (m_isLeakyPrev != 3 && m_isLeaky == 3) m_successiveLeakyIntvls = 1;
   else if (m_isLeakyPrev == 3 && m_isLeaky != 3) m_successiveLeakyIntvls = 0;

   return (m_isLeaky);
}

void MLMProcess::LogMemError(unsigned short leaky)
{
   if (m_memSizeDataSz == 0) return;
   //convert 16-bit binary fraction to decimal
   unsigned long pctval = m_currPercentMem;
   pctval = (pctval * 1000) >> 15;
   MLM_LOG_ERROR4(m_log, "Potential MLK: pid=%u, %%sysmem=%3u.%u, memusage=%u", m_pid, pctval/10, pctval % 10, m_memSizeData[m_memSizeDataSz-1]);
   MLM_LOG_ERROR2(m_log, "Potential MLK: current_avg=%u, previous_avg=%u", m_memUsageAvg, m_memUsageAvgPrev);

   /*
    * Calculate the start and end times of the invocation interval.
    *
    * The end time is simple - it's the current time.
    * The start time is the current time minus the tolerance interval (number
    * of invocations) times 60 (the number of seconds) times the # of
    * successive leaky intervals.
    */
   time_t now = time(0);
   MLMConfiguration& config = MLMConfiguration::Instance();
   time_t start = now - config.ToleranceIntvl() * 60 * m_successiveLeakyIntvls;
   std::string tolerance("Tolerance interval: from ");
   char timeBuf[512];
   const char * const fmt = "%a %b %d %T %Z %Y";
   strftime(timeBuf, sizeof(timeBuf), fmt, localtime(&start));
   tolerance += timeBuf;
   tolerance += " to ";
   strftime(timeBuf, sizeof(timeBuf), fmt, localtime(&now));
   tolerance += timeBuf;
   //delete 2 lines below by shuwenli to suppport MSI
   //MLMNotification::getNotificationAgent().sendNotification(m_pid, m_name,
   //   m_memSizeData[m_memSizeDataSz-1], tolerance);
}

void MLMProcess::LogMemWarning(unsigned short leaky)
{
   //convert 16-bit binary fraction to decimal
   unsigned long pctval = m_currPercentMem;
   pctval = (pctval * 1000) >> 15;
   MLM_LOG_DEBUG4(m_log, MLM_DEBUG_MIN, "WARNING: Potential MLK: pid=%u, %%sysmem=%3u.%u, memusage=%u", m_pid, pctval/10, pctval % 10, m_memSizeData[m_memSizeDataSz-1]);
   MLM_LOG_DEBUG2(m_log, MLM_DEBUG_MIN, "WARNING: Potential MLK: current_avg=%u, previous_avg=%u", m_memUsageAvg, m_memUsageAvgPrev);
   return;
}

void MLMProcess::ResetToleranceInterval(void)
{
   m_memSizeDataSz = 0;
   m_completedToleranceSlots = 0;
   m_memUsageAvgPrev = m_memUsageAvg;
   m_memUsageAvg = 0;
   m_isLeakyPrev = m_isLeaky;
   m_isLeaky = 0;
   
   return;
}

const unsigned long MLMProcess::GetMemSize(void) const
{
   if (m_memSizeDataSz) return (m_memSizeData[m_memSizeDataSz - 1]);
   else return (0);
}

double MLMProcess::CalculateTrendFactor(void)
{
   double sigX = 0;
   double sigY = 0;
   double sigXY = 0;
   double sigXX = 0;
   double sigYY = 0;

   for (unsigned short i=0; i<m_memSizeDataSz; ++i) {
      sigX += ((double)i+1);
      sigY += ((double)m_memSizeData[i]);
      sigXY += ((double)i+1)*((double)m_memSizeData[i]);
      sigXX += ((double)i*(double)i)+(2*(double)i)+1;
      sigYY += ((double)m_memSizeData[i])*((double)m_memSizeData[i]);
   }

   double r_num = (m_memSizeDataSz*sigXY) - (sigX*sigY);
   double r_den2 = ((m_memSizeDataSz*sigXX) - (sigX*sigX))*((m_memSizeDataSz*sigYY) - (sigY*sigY));
   double r_den = sqrt(r_den2);
   double r = 0;
   if (r_den) r = r_num/r_den;

   return r;
}
