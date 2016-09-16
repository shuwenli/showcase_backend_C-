/************************************************************************/
/*                                                                      */
/*  COPYRIGHT (c) 2006 Alcatel-Lucent                                   */
/*  All Rights Reserved.                                                */
/*                                                                      */
/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF LUCENT TECHNOLOGIES  */
/*                                                                      */
/*  The copyright notice above does not evidence any                    */
/*  actual or intended publication of such source code.                 */
/*                                                                      */
/************************************************************************/
#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <iostream.h>
#include <time.h>
#include <limits.h>

#include "MLMLogger.h"
#include "MLMConfiguration.h"
#include "MLMConstants.h"

MLMLogger * MLMLogger::sm_instance = 0;
char MLMLogger::sm_buffer[LOGGER_BUFSIZE];

MLMLogger::MLMLogger() : m_debugLevel(DebugLevel_g), m_errorSize(0), m_maxErrorSize(MaxErrorLogfileSize_g), m_mapSize(0), 
   m_maxMapSize(MaxMapLogfileSize_g), m_usageSize(0), m_maxUsageSize(MaxUsageLogfileSize_g), m_isConfigDataLocked(false), 
   m_logBufPtr(0), m_config(MLMConfiguration::Instance())
{
   m_logBuf1[0] = 0;
   m_logBuf2[0] = 0;
   m_logBuf3[0] = 0;

   ConfigChanged(MaskBit_Log_Directory|MaskBit_Error_Logfile|MaskBit_Map_Logfile|MaskBit_Usage_Logfile|MaskBit_Maximum_Error_Logfile_Size|MaskBit_Maximum_Map_Logfile_Size|MaskBit_Maximum_Usage_Logfile_Size|MaskBit_Debug_Level);

   m_config.RegisterForConfigChanges(this);

   MLM_LOG_DEBUG1((*this), MLM_DEBUG_MEM, "Created MLMLogger object of size %d bytes", sizeof(*this));
}

MLMLogger & MLMLogger::Instance(void)
{
   if (MLMLogger::sm_instance == 0) {
      MLMLogger::sm_instance = new MLMLogger;
      MLMLogger::sm_buffer[0] = 0;
   }
   return (*MLMLogger::sm_instance);
}

MLMLogger::~MLMLogger()
{
   MLM_LOG_DEBUG1((*this), MLM_DEBUG_MEM, "Destroyed MLMLogger object of size %d bytes", sizeof(*this));

   if (m_errorStrm.is_open()) m_errorStrm.close();
   if (m_mapStrm.is_open()) m_mapStrm.close();
   if (m_usageStrm.is_open()) m_usageStrm.close();

   delete MLMLogger::sm_instance;
   MLMLogger::sm_instance = 0;
}

bool MLMLogger::IsEnabledForLevel(unsigned int lvl) 
{ 
   if (lvl == MLM_DEBUG_MEM && m_debugLevel == MLM_DEBUG_MEM) return (true);
   if (lvl > MLM_DEBUG_NONE && lvl <= MLM_DEBUG_MAX && lvl <= m_debugLevel) return (true); 
   if (lvl == MLM_DEBUG_NONE && lvl == m_debugLevel) return (false);
   else return (false); 
}

unsigned int MLMLogger::ChangeDebugLevel(unsigned int lvl)
{
   if (lvl <= MLM_DEBUG_MAX) m_debugLevel = lvl;
   if (lvl == MLM_DEBUG_MEM) m_debugLevel = lvl;
   return (m_debugLevel);
}


void MLMLogger::LogError(const char * msg)
{  
   static unsigned int splitLen;
   static std::string splitMsg;

   if (!m_errorStrm.good()) return;

   //check file length
   //also use the length of the timestamp prefix (as below) in the file length calculation
   //strlen("YYYY MTH DD hh:mm:ss: ERROR: ") + 1= 29 + 1 = 30
   if ((m_errorSize + strlen(msg) + 30) > m_maxErrorSize) { 
      if ((m_errorSize + 30) >= m_maxErrorSize) {
         //not enough space to accomodate part of the msg
         RolloverErrorLogFile();
         WriteToErrorLogFile(msg, "ERROR: ");
      } else {
         //part of the msg can be logged, split msg across files
         splitLen = m_maxErrorSize - m_errorSize - 30;
         splitMsg = msg;
         splitMsg.erase(splitLen);
         WriteToErrorLogFile(splitMsg.c_str(), "ERROR: ");
         RolloverErrorLogFile();
         splitMsg = (msg + splitLen);
         WriteToErrorLogFile(splitMsg.c_str(), "ERROR: ");
      }
   } else {
      //enough space to log entire msg
      WriteToErrorLogFile(msg, "ERROR: ");
   }

   return;
}

void MLMLogger::LogDebug(unsigned int lvl, const char * msg)
{
   static unsigned int splitLen;
   static std::string splitMsg;

   if (!IsEnabledForLevel(lvl)) return;
   if (!m_errorStrm.good()) return;

   //check file length
   //also use the length of the timestamp prefix (as below) in the file length calculation
   //strlen("YYYY MTH DD hh:mm:ss: ") + 1 = 22 + 1 = 23
   if ((m_errorSize + strlen(msg) + 23) > m_maxErrorSize) { 
         //not enough space to accomodate part of the msg
      if ((m_errorSize + 23) >= m_maxErrorSize) {
         RolloverErrorLogFile();
         WriteToErrorLogFile(msg, "");
      } else {
         //part of the msg can be logged, split msg across files
         splitLen = m_maxErrorSize - m_errorSize - 23;
         splitMsg = msg;
         splitMsg.erase(splitLen);
         WriteToErrorLogFile(splitMsg.c_str(), "");
         RolloverErrorLogFile();
         splitMsg = (msg + splitLen);
         WriteToErrorLogFile(splitMsg.c_str(), "");
      }
   } else {
      //enough space to log entire msg
      WriteToErrorLogFile(msg, "");
   }

   return;
}

void MLMLogger::LogMap(char * msg)
{
   static unsigned int splitLen;
   static std::string splitMsg;

   if (!m_mapStrm.good()) return;
   
   //check file length
   //also use the length of the timestamp prefix (as below) in the file length calculation
   //strlen("YYYY MTH DD hh:mm:ss: ") + 1 = 22 + 1 = 23
   if ((m_mapSize + strlen(msg) + 23) > m_maxMapSize) { 
         //not enough space to accomodate part of the msg
      if ((m_mapSize + 23) >= m_maxMapSize) {
         RolloverMapLogFile();
         WriteToMapLogFile(msg);
      } else {
         //part of the msg can be logged, split msg across files
         splitLen = m_maxMapSize - m_mapSize - 23;
         splitMsg = msg;
         splitMsg.erase(splitLen);
         WriteToMapLogFile(splitMsg.c_str());
         RolloverMapLogFile();
         splitMsg = (msg + splitLen);
         WriteToMapLogFile(splitMsg.c_str());
      }
   } else {
      //enough space to log entire msg
      WriteToMapLogFile(msg);
   }

   return;
}

void MLMLogger::StartLogUsage(void)
{
   m_logBufPtr = m_logBuf3;
   return;
}

void MLMLogger::EndLogUsage(void)
{
   static unsigned int splitLen;
   static std::string splitMsg;

   if (!m_usageStrm.good()) return;
   
   //check file length
   if ((m_usageSize + strlen(m_logBuf3) + 1) > m_maxUsageSize) { 
         //not enough space to accomodate part of the m_logBuf3
      if ((m_usageSize + 1) >= m_maxUsageSize) {
         RolloverUsageLogFile();
         WriteToUsageLogFile(m_logBuf3);
      } else {
         //part of the m_logBuf3 can be logged, split m_logBuf3 across files
         splitLen = m_maxUsageSize - m_usageSize - 1;
         splitMsg = m_logBuf3;
         splitMsg.erase(splitLen);
         WriteToUsageLogFile(splitMsg.c_str());
         RolloverUsageLogFile();
         splitMsg = (m_logBuf3 + splitLen);
         WriteToUsageLogFile(splitMsg.c_str());
      }
   } else {
      //enough space to log entire m_logBuf3
      WriteToUsageLogFile(m_logBuf3);
   }

   m_logBufPtr = 0;
   return;
}

void MLMLogger::LogUsage(const time_t * tptr, unsigned long pid, unsigned long usage)
{
   static struct tm tmv;
   static unsigned short nlen = 0;

   if ((m_logBuf3 + (LOGGER_USAGE_BUFSIZE -1) - m_logBufPtr) < 32) {
      EndLogUsage();
      StartLogUsage();
      *m_logBufPtr++ = '-'; //a continuation log line starts with '-'
   }

   localtime_r(tptr, &tmv);
   nlen = sprintf(m_logBufPtr, "%.2d%.2d%.2d%.2d%.2d%.2d:%u:%u;", tmv.tm_year-100, tmv.tm_mon+1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec, pid, usage);
   m_logBufPtr += nlen;

   return;
}

void MLMLogger::ConfigChanged(unsigned int changedMask)
{
   if (!m_isConfigDataLocked && (changedMask & ( MaskBit_Log_Directory | 
                                                 MaskBit_Error_Logfile |
                                                 MaskBit_Map_Logfile |
                                                 MaskBit_Usage_Logfile |
                                                 MaskBit_Maximum_Error_Logfile_Size |
                                                 MaskBit_Maximum_Map_Logfile_Size |
                                                 MaskBit_Maximum_Usage_Logfile_Size ))) {
      //dynamic changes to log file/dir and size configuration are still allowed
      //since these are meta-dynamic config parameters

      if ((changedMask & MaskBit_Log_Directory) && m_config.m_invalidLogD[0] != 0 ) {
            MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Specified Log_Directory %s is invalid", m_config.m_invalidLogD);
            MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Log_Directory is %s", m_config.LogDir());
      }

      if ((changedMask & MaskBit_Log_Directory) || (changedMask & MaskBit_Error_Logfile)) {
         m_errorLogfileName = m_config.LogDir();
         m_errorLogfileName += "/";
         m_errorLogfileName += m_config.ErrorLogfileName();
         OpenErrorLogFile();
         if (m_config.m_invalidErrL[0] != 0) {
            MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Specified Error_Logfile %s is invalid", m_config.m_invalidErrL);
         }
         MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Error_Logfile is %s", m_errorLogfileName.c_str());
      }

      if ((changedMask & MaskBit_Log_Directory) || (changedMask & MaskBit_Map_Logfile)) {
         m_mapLogfileName = m_config.LogDir();
         m_mapLogfileName += "/";
         m_mapLogfileName += m_config.MapLogfileName();
         if (m_config.m_invalidMapL[0] != 0) {
            MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Specified Map_Logfile %s is invalid", m_config.m_invalidMapL);
         }
         MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Map_Logfile is %s", m_mapLogfileName.c_str());
         OpenMapLogFile();
      }

      if ((changedMask & MaskBit_Log_Directory) || (changedMask & MaskBit_Usage_Logfile)) {
         m_usageLogfileName = m_config.LogDir();
         m_usageLogfileName += "/";
         m_usageLogfileName += m_config.UsageLogfileName();
         if (m_config.m_invalidUsgL[0] != 0) {
            MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Specified Usage_Logfile %s is invalid", m_config.m_invalidUsgL);
         }
         MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Usage_Logfile is %s", m_usageLogfileName.c_str());
         OpenUsageLogFile();
      }

      if (changedMask & MaskBit_Maximum_Error_Logfile_Size) {
         m_maxErrorSize = m_config.MaxErrorLogfileSize();
         if (m_config.m_invalidErrS[0] != 0) {
            MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Specified Maximum_Error_Logfile_Size %s is invalid", m_config.m_invalidErrS);
         }
         MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Maximum_Error_Logfile_Size is %u bytes", m_maxErrorSize);
      }
            
      if (changedMask & MaskBit_Maximum_Map_Logfile_Size) {
         m_maxMapSize = m_config.MaxMapLogfileSize();
         if (m_config.m_invalidMapS[0] != 0) {
            MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Specified Maximum_Map_Logfile_Size %s is invalid", m_config.m_invalidMapS);
         }
         MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Maximum_Map_Logfile_Size is %u bytes", m_maxMapSize);
      }
            
      if (changedMask & MaskBit_Maximum_Usage_Logfile_Size) {
         m_maxUsageSize = m_config.MaxUsageLogfileSize();
         if (m_config.m_invalidUsgS[0] != 0) {
            MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Specified Maximum_Usage_Logfile_Size %s is invalid", m_config.m_invalidUsgS);
         }
         MLM_LOG_DEBUG1((*this), MLM_DEBUG_MIN, "Maximum_Usage_Logfile_Size is %u bytes", m_maxUsageSize);
      }
   }

   if (changedMask & MaskBit_Debug_Level) {
      //dynamic changes to debug level are always allowed
      ChangeDebugLevel(m_config.DebugLevel());
      MLM_LOG_DEBUG1((*this), MLM_DEBUG_4, "Debug_Level is %u", m_debugLevel);
   }
   return;
}

int MLMLogger::FormatTime(char * buf, struct tm tmv)
{
    static char buf2[15];

    sprintf(buf, "%.4d ", 1900 + tmv.tm_year);
    switch(tmv.tm_mon) {
       case 0:  strcat(buf,"Jan"); break;
       case 1:  strcat(buf,"Feb"); break;
       case 2:  strcat(buf,"Mar"); break;
       case 3:  strcat(buf,"Apr"); break;
       case 4:  strcat(buf,"May"); break;
       case 5:  strcat(buf,"Jun"); break;
       case 6:  strcat(buf,"Jul"); break;
       case 7:  strcat(buf,"Aug"); break;
       case 8:  strcat(buf,"Sep"); break;
       case 9:  strcat(buf,"Oct"); break;
       case 10: strcat(buf,"Nov"); break;
       case 11: strcat(buf,"Dec"); break;
       default: break;
    }
    sprintf(buf2, " %.2d %.2d:%.2d:%.2d", tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    strcat(buf, buf2);

    return (strlen(buf));
}

void MLMLogger::GetErrorLogFilename(char * fbuf)
{
   strcpy(fbuf, m_errorLogfileName.c_str());
   return; 
}

void MLMLogger::OpenErrorLogFile(void)
{
   static long initialpos, finalpos;
   if (m_errorStrm.is_open()) m_errorStrm.close();

   m_errorStrm.open(m_errorLogfileName.c_str(), std::ios::out|std::ios::app);
   if (!m_errorStrm.good()) {
      cout << "ERROR: Cannot open error log file " << m_errorLogfileName.c_str() << endl;
   } else {
      m_errorStrm.seekp(0, std::ios::end);
      initialpos = m_errorStrm.tellp();
      if (initialpos >= 0) {
         m_errorSize = initialpos;
      }

      //write a newline to the file to start with fresh
      m_errorStrm << std::endl;
      finalpos = m_errorStrm.tellp();
      if (initialpos >=0 && finalpos >=0 && finalpos >= initialpos) {
         m_errorSize += (finalpos - initialpos);
      }
   }

   return;
}

void MLMLogger::WriteToErrorLogFile(const char * msg, const char * prefix)
{
   static time_t rawtime;
   static struct tm timeinfo;
   static long initialpos, finalpos;

   time(&rawtime);
   localtime_r(&rawtime, &timeinfo);
   MLMLogger::FormatTime(m_logBuf1, timeinfo);
   
   initialpos = m_errorStrm.tellp();
   m_errorStrm << m_logBuf1 << ": " << prefix << msg << std::endl;
   finalpos = m_errorStrm.tellp();

   if (initialpos >=0 && finalpos >=0 && finalpos >= initialpos) {
      m_errorSize += (finalpos - initialpos);
   }
   
   return;
}

void MLMLogger::RolloverErrorLogFile(void)
{
   static char oldfile[PATH_MAX+1];
   static char currfile[PATH_MAX+1];

   if (m_errorStrm.is_open()) m_errorStrm.close();

   GetErrorLogFilename(oldfile);
   strcpy(currfile, oldfile);
   strcat(currfile, ".old");
   rename(oldfile, currfile);

   OpenErrorLogFile();

   return;
}

void MLMLogger::GetMapLogFilename(char * fbuf)
{
   strcpy(fbuf, m_mapLogfileName.c_str());
   return; 
}

void MLMLogger::OpenMapLogFile(void)
{
   static long initialpos;

   if (m_mapStrm.is_open()) m_mapStrm.close();

   m_mapStrm.open(m_mapLogfileName.c_str(), std::ios::out|std::ios::app);
   if (!m_mapStrm.good()) {
      MLM_LOG_ERROR1((*this), "Cannot open map log file %s", m_mapLogfileName.c_str());
   } else {
      m_mapStrm.seekp(0, std::ios::end);
      initialpos = m_mapStrm.tellp();
      if (initialpos >= 0) {
         m_mapSize = initialpos;
      }
   }

   return;
}

void MLMLogger::WriteToMapLogFile(const char * msg)
{
   static time_t rawtime;
   static struct tm timeinfo;
   static long initialpos, finalpos;

   time(&rawtime);
   localtime_r(&rawtime, &timeinfo);
   MLMLogger::FormatTime(m_logBuf2, timeinfo);
   
   initialpos = m_mapStrm.tellp();
   m_mapStrm << m_logBuf2 << ": " << msg << std::endl;
   finalpos = m_mapStrm.tellp();

   if (initialpos >=0 && finalpos >=0 && finalpos >= initialpos) {
      m_mapSize += (finalpos - initialpos);
   }
   
   return;
}

void MLMLogger::RolloverMapLogFile(void)
{
   static char oldfile[PATH_MAX+1];
   static char currfile[PATH_MAX+1];

   if (m_mapStrm.is_open()) m_mapStrm.close();

   GetMapLogFilename(oldfile);
   strcpy(currfile, oldfile);
   strcat(currfile, ".old");
   rename(oldfile, currfile);

   OpenMapLogFile();

   return;
}

void MLMLogger::GetUsageLogFilename(char * fbuf)
{
   strcpy(fbuf, m_usageLogfileName.c_str());
   return; 
}

void MLMLogger::OpenUsageLogFile(void)
{
   static long initialpos;

   if (m_usageStrm.is_open()) m_usageStrm.close();

   m_usageStrm.open(m_usageLogfileName.c_str(), std::ios::out|std::ios::app);
   if (!m_usageStrm.good()) {
      MLM_LOG_ERROR1((*this), "Cannot open usage log file %s", m_usageLogfileName.c_str());
   } else {
      m_usageStrm.seekp(0, std::ios::end);
      initialpos = m_usageStrm.tellp();
      if (initialpos >= 0) {
         m_usageSize = initialpos;
      }
   }

   return;
}

void MLMLogger::WriteToUsageLogFile(const char * msg)
{
   static long initialpos, finalpos;

   initialpos = m_usageStrm.tellp();
   m_usageStrm << msg << std::endl;
   finalpos = m_usageStrm.tellp();

   if (initialpos >=0 && finalpos >=0 && finalpos >= initialpos) {
      m_usageSize += (finalpos - initialpos);
   }
   
   return;
}

void MLMLogger::RolloverUsageLogFile(void)
{
   static char oldfile[PATH_MAX+1];
   static char currfile[PATH_MAX+1];

   if (m_usageStrm.is_open()) m_usageStrm.close();

   GetUsageLogFilename(oldfile);
   strcpy(currfile, oldfile);
   strcat(currfile, ".old");
   rename(oldfile, currfile);

   OpenUsageLogFile();

   return;
}

#ifdef _MLM_TEST_LOGGER_

int main (void)
{
   cout << "Program to test class MLMLogger" << endl;
   cout << "================================================================" << endl;

   MLMLogger & logger = MLMLogger::Instance();

   char fname[PATH_MAX+1];
   logger.GetErrorLogFilename(fname);
   cout << "Error log file is " << fname << endl;
   cout << "Max. error log size is " << logger.GetMaxErrorLogSize() << " bytes" << endl;
   cout << "Current error log size is " << logger.GetErrorLogSize() << " bytes" << endl;
   logger.GetMapLogFilename(fname);
   cout << "Map log file is " << fname << endl;
   cout << "Max. map log size is " << logger.GetMaxMapLogSize() << " bytes" << endl;
   cout << "Current map log size is " << logger.GetMapLogSize() << " bytes" << endl;
   logger.GetUsageLogFilename(fname);
   cout << "Usage log file is " << fname << endl;
   cout << "Max. usage log size is " << logger.GetMaxUsageLogSize() << " bytes" << endl;
   cout << "Current usage log size is " << logger.GetUsageLogSize() << " bytes" << endl;
   cout << "================================================================" << endl;

   cout << "Change log file/dir parameters and then type 'y' and hit enter to continue..." << endl;
   char c;
   cin >> c;
   MLMConfiguration::Instance().Read();

   logger.GetErrorLogFilename(fname);
   cout << "Error log file is " << fname << endl;
   cout << "Max. error log size is " << logger.GetMaxErrorLogSize() << " bytes" << endl;
   cout << "Current error log size is " << logger.GetErrorLogSize() << " bytes" << endl;
   logger.GetMapLogFilename(fname);
   cout << "Map log file is " << fname << endl;
   cout << "Max. map log size is " << logger.GetMaxMapLogSize() << " bytes" << endl;
   cout << "Current map log size is " << logger.GetMapLogSize() << " bytes" << endl;
   logger.GetUsageLogFilename(fname);
   cout << "Usage log file is " << fname << endl;
   cout << "Max. usage log size is " << logger.GetMaxUsageLogSize() << " bytes" << endl;
   cout << "Current usage log size is " << logger.GetUsageLogSize() << " bytes" << endl;
   cout << "================================================================" << endl;

   cout << "Test error logging" << endl;
   logger.LogError("This is an error msg");
   cout << "Log File size is now " << logger.GetErrorLogSize() << " bytes" << endl;
   cout << "================================================================" << endl;

   cout << "Test debug logging" << endl;
   logger.LogDebug(MLM_DEBUG_MIN, "This is a debug msg");
   cout << "Log File size is now " << logger.GetErrorLogSize() << " bytes" << endl;
   cout << "================================================================" << endl;

   cout << "Test for MLMLogger::FormatTime()" << endl;
   struct tm tmv;
   tmv.tm_year = 105;
   tmv.tm_mon = 4;
   tmv.tm_mday = 1;
   tmv.tm_hour = 16;
   tmv.tm_min = 9;
   tmv.tm_sec = 6;
   char buf[30];
   MLMLogger::FormatTime(buf, tmv);
   cout << "Formatted time for 5/1/2005 16:09:06 is " << buf << endl;
   cout << "================================================================" << endl;

   cout << "Test MACROS" << endl;
   logger.ChangeDebugLevel(MLM_DEBUG_NONE);
   logger.LogError("Debug Level set to MLM_DEBUG_NONE, Msgs 1-5 should not print");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_MIN, "Msg1 Debug Level MLM_DEBUG_MIN Msg");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_2, "Msg2 Debug Level MLM_DEBUG_2 Msg");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_3, "Msg3 Debug Level MLM_DEBUG_3 Msg");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_4, "Msg4 Debug Level MLM_DEBUG_4 Msg");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_MAX, "Msg5 Debug Level MLM_DEBUG_MAX Msg");

   logger.ChangeDebugLevel(MLM_DEBUG_4);
   logger.LogError("Debug Level set to MLM_DEBUG_4");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_MIN, "Msg6 Debug Level MLM_DEBUG_MIN Msg");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_2, "Msg7 Debug Level MLM_DEBUG_2 Msg");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_3, "Msg8 Debug Level MLM_DEBUG_3 Msg");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_4, "Msg9 Debug Level MLM_DEBUG_4 Msg");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_4, "Msg10: This is a Debug Level MLM_DEBUG_4 Msg, The next msg (Msg11) with debug level MLM_DEBUG_MAX should not print");
   MLM_LOG_DEBUG(logger, MLM_DEBUG_MAX, "Msg11 Debug Level MLM_DEBUG_MAX Msg");
   
   logger.LogError("Debug Logging with params");
   logger.ChangeDebugLevel(MLM_DEBUG_MAX);
   logger.LogError("Debug Level set to MLM_DEBUG_MAX");
   MLM_LOG_DEBUG1(logger, 1, "Msg12 Debug Level MLM_DEBUG_MIN with 1 param, value=%u", 1);
   MLM_LOG_DEBUG2(logger, 2, "Msg13 Debug Level MLM_DEBUG_2 with 2 params, value=%u, value=%s", 1, "msg");
   MLM_LOG_DEBUG3(logger, 3, "Msg14 Debug Level MLM_DEBUG_3 with 3 params, value=%u, value=%s, value=%u", 1, "msg", 3);
   MLM_LOG_DEBUG4(logger, 4, "Msg15 Debug Level MLM_DEBUG_4 with 4 params, value=%u, value=%s, value=%u, value=%s", 1, "msg", 3, "msg");
   MLM_LOG_DEBUG4(logger, 5, "Msg16 Debug Level MLM_DEBUG_MAX with 4 params, value=%u, value=%s, value=%u, value=%s", 1, "msg", 3, "msg");

   logger.LogError("Error Logging with params");
   MLM_LOG_ERROR1(logger, "Msg17 Error Msg with 1 param, value=%u", 1);
   MLM_LOG_ERROR2(logger, "Msg18 Error Msg with 2 params, value=%u, value=%s", 1, "msg");
   MLM_LOG_ERROR3(logger, "Msg19 Error Msg with 3 params, value=%u, value=%s, value=%u", 1, "msg", 3);
   MLM_LOG_ERROR4(logger, "Msg20 Error Msg with 4 params, value=%u, value=%s, value=%u, value=%s", 1, "msg", 3, "msg");
   return (0);
}

#endif
