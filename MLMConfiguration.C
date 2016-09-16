/* -*- c++ -*- */ /* vim:set ts=3 sts=3 sw=3 et syntax=cpp: */
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
#include <stdio.h>
#include <fstream>
#include <iostream.h>
#include <sys/types.h>
#include <dirent.h>

#include "MLMConstants.h"
#include "MLMConfiguration.h"

MLMConfiguration * MLMConfiguration::sm_instance = 0;
extern char * sys_errlist[];
extern int sys_nerr;

MLMConfiguration::MLMConfiguration() : m_invocationIntvl(InvocationIntvl_g), m_toleranceIntvl(ToleranceIntvl_g), 
   m_enabled(true), m_debugLevel(DebugLevel_g), m_initializationIntvl(InitializationIntvl_g), 
   m_allowableTolCount(AllowableTolCount_g), m_logDir(MLMConstants::LogDir_g),
   m_errorLogfileName(MLMConstants::ErrorLogfileName_g), 
   m_mapLogfileName(MLMConstants::MapLogfileName_g),
   m_usageLogfileName(MLMConstants::UsageLogfileName_g), 
   m_maxErrorLogfileSize(MaxErrorLogfileSize_g), m_maxMapLogfileSize(MaxMapLogfileSize_g), 
   m_maxUsageLogfileSize(MaxUsageLogfileSize_g), m_changedMask(0)
{
   MLMConfiguration::sm_instance = this;
	m_configFile[0] = 0;

   m_invalidIniIntvl[0] = 0;
   m_invalidInvIntvl[0] = 0;
   m_invalidTolIntvl[0] = 0;
   m_invalidATC[0] = 0;
   m_invalidEnab[0] = 0;
   m_invalidLogD[0] = 0;
   m_invalidErrL[0] = 0;
   m_invalidUsgL[0] = 0;
   m_invalidMapL[0] = 0;
   m_invalidErrS[0] = 0;
   m_invalidUsgS[0] = 0;
   m_invalidMapS[0] = 0;

   GetConfigFile();
   Read();
}

MLMConfiguration::~MLMConfiguration()
{
   m_changeCbkRegistry.erase(m_changeCbkRegistry.begin(), m_changeCbkRegistry.end());

   delete MLMConfiguration::sm_instance;
   MLMConfiguration::sm_instance = 0;
}

MLMConfiguration & MLMConfiguration::Instance(void)
{
   if (MLMConfiguration::sm_instance == 0) {
      MLMConfiguration::sm_instance = new MLMConfiguration;
   }
   return (*MLMConfiguration::sm_instance);
}

void MLMConfiguration::GetConfigFile(void)
{
   std::ifstream testCfg;
	testCfg.open(MLMConstants::ConfigFileAppl_g, std::ifstream::in);
	testCfg.close();
	if (testCfg.fail()) {
      //ConfigFileAppl_g does not exist, use default
      testCfg.clear(ios::failbit);
		strcpy(m_configFile, MLMConstants::ConfigFile_g);
   } else {
      //ConfigFileAppl_g exists, use it
      strcpy(m_configFile, MLMConstants::ConfigFileAppl_g);
   }
}

const unsigned int MLMConfiguration::Read(void)
{
   m_changedMask = 0;

   std::ifstream configFile;
   configFile.open(m_configFile);

   std::string line, name, value;
   int posEqual;

   m_invalidIniIntvl[0] = 0;
   m_invalidInvIntvl[0] = 0;
   m_invalidTolIntvl[0] = 0;
   m_invalidATC[0] = 0;
   m_invalidEnab[0] = 0;
   m_invalidLogD[0] = 0;
   m_invalidErrL[0] = 0;
   m_invalidUsgL[0] = 0;
   m_invalidMapL[0] = 0;
   m_invalidErrS[0] = 0;
   m_invalidUsgS[0] = 0;
   m_invalidMapS[0] = 0;


   while (std::getline(configFile, line)) {
      if (! line.length()) continue;
      if (line[0] == '#') continue; 
      if (line[0] == ';') continue; 

      posEqual = line.find('=');
      name = MLMConfiguration::Trim(line.substr(0, posEqual));
      value = MLMConfiguration::Trim(line.substr(posEqual+1));

      unsigned long tmpval = 0;
      long stmpval = 0;
      bool tmpbool = false;
      std::string tmpstr;

      /****/ if (name == "Initialization_Interval") {
         stmpval = atol(value.c_str());
         if (stmpval < (long) InitializationIntvlMin_g || stmpval > (long) InitializationIntvlMax_g) {
            stmpval = InitializationIntvl_g;
            strncpy(m_invalidIniIntvl, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidIniIntvl[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (m_initializationIntvl != stmpval) {
            m_initializationIntvl = stmpval;
            m_changedMask |= MaskBit_Initialization_Interval;
         }
      } else if (name == "Invocation_Interval") {
         stmpval = atol(value.c_str());
         if (stmpval < InvocationIntvlMin_g || stmpval > InvocationIntvlMax_g) {
            stmpval = InvocationIntvl_g;
            strncpy(m_invalidInvIntvl, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidInvIntvl[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (m_invocationIntvl != stmpval) {
            m_invocationIntvl = stmpval;
            m_changedMask |= MaskBit_Invocation_Interval;
         }
      } else if (name == "Tolerance_Interval") {
         stmpval = atol(value.c_str());
         if (stmpval < ToleranceIntvlMin_g || stmpval > ToleranceIntvlMax_g) {
            stmpval = ToleranceIntvl_g;
            strncpy(m_invalidTolIntvl, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidTolIntvl[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (m_toleranceIntvl != stmpval) {
            m_toleranceIntvl = stmpval;
            m_changedMask |= MaskBit_Tolerance_Interval;
         }
      } else if (name == "Allowable_Tolerance_Count") {
         stmpval = atol(value.c_str());
         if (stmpval < AllowableTolCountMin_g || stmpval > AllowableTolCountMax_g) {
            stmpval = AllowableTolCount_g;
            strncpy(m_invalidATC, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidATC[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (m_allowableTolCount != stmpval) {
            m_allowableTolCount = stmpval;
            m_changedMask |= MaskBit_Allowable_Tolerance_Count;
         }
      } else if (name == "Enable_MLM") {
         stmpval = atol(value.c_str());
         if (stmpval < 0 || stmpval > 1) {
            stmpval = 1;
            strncpy(m_invalidEnab, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidEnab[INVALID_CONFIG_DATA_LEN] = 0;
         }
         tmpbool = (stmpval == 1) ? true : false;
         if (tmpbool != m_enabled) {
            m_enabled = tmpbool;
            m_changedMask |= MaskBit_Enable_MLM;
         }
      } else if (name == "Log_Directory") {
         tmpstr = value;
         
         //remove trailing '/' from the directory-name (if any)
         unsigned int len = tmpstr.length();
         if (len == 0 || len > LogDirMaxLength) { 
            tmpstr = MLMConstants::LogDir_g; 
            strncpy(m_invalidLogD, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidLogD[INVALID_CONFIG_DATA_LEN] = 0;
         } else if (tmpstr.find_last_of("/") == (len-1)) {
            tmpstr.erase(len-1);
            strncpy(m_invalidLogD, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidLogD[INVALID_CONFIG_DATA_LEN] = 0;
         }
         std::string valid_dir;
         ValidateDir(tmpstr, valid_dir);
         if (tmpstr != valid_dir) {
            strncpy(m_invalidLogD, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidLogD[INVALID_CONFIG_DATA_LEN] = 0;
         }
         tmpstr = valid_dir;

         if (tmpstr != m_logDir) {
            m_logDir = tmpstr;
            m_changedMask |= MaskBit_Log_Directory;
         }
      } else if (name == "Error_Logfile") {
         tmpstr = value;
         unsigned int len = tmpstr.length();
         if (len == 0 || len > LogfileNameMaxLength) {
            tmpstr = MLMConstants::ErrorLogfileName_g;
            strncpy(m_invalidErrL, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidErrL[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (tmpstr != m_errorLogfileName) {
            m_errorLogfileName = tmpstr;
            m_changedMask |= MaskBit_Error_Logfile;
         }
      } else if (name == "Map_Logfile") {
         tmpstr = value;
         unsigned int len = tmpstr.length();
         if (len == 0 || len > LogfileNameMaxLength) {
            tmpstr = MLMConstants::MapLogfileName_g;
            strncpy(m_invalidMapL, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidMapL[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (tmpstr != m_mapLogfileName) {
            m_mapLogfileName = tmpstr;
            m_changedMask |= MaskBit_Map_Logfile;
         }
      } else if (name == "Usage_Logfile") {
         tmpstr = value;
         unsigned int len = tmpstr.length();
         if (len == 0 || len > LogfileNameMaxLength) {
            tmpstr = MLMConstants::UsageLogfileName_g;
            strncpy(m_invalidUsgL, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidUsgL[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (tmpstr != m_usageLogfileName) {
            m_usageLogfileName = tmpstr;
            m_changedMask |= MaskBit_Usage_Logfile;
         }
      } else if (name == "Maximum_Error_Logfile_Size") {
         stmpval = atol(value.c_str());
         if (stmpval < MaxErrorLogfileSizeMin_g || stmpval > MaxErrorLogfileSizeMax_g) {
            stmpval = MaxErrorLogfileSize_g;
            strncpy(m_invalidErrS, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidErrS[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (m_maxErrorLogfileSize != stmpval) {
            m_maxErrorLogfileSize = stmpval;
            m_changedMask |= MaskBit_Maximum_Error_Logfile_Size;
         }
      } else if (name == "Maximum_Map_Logfile_Size") {
         stmpval = atol(value.c_str());
         if (stmpval < MaxMapLogfileSizeMin_g || stmpval > MaxMapLogfileSizeMax_g) {
            stmpval = MaxMapLogfileSize_g;
            strncpy(m_invalidMapS, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidMapS[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (m_maxMapLogfileSize != stmpval) {
            m_maxMapLogfileSize = stmpval;
            m_changedMask |= MaskBit_Maximum_Map_Logfile_Size;
         }
      } else if (name == "Maximum_Usage_Logfile_Size") {
         stmpval = atol(value.c_str());
         if (stmpval < MaxUsageLogfileSizeMin_g || stmpval > MaxUsageLogfileSizeMax_g) {
            stmpval = MaxUsageLogfileSize_g;
            strncpy(m_invalidUsgS, value.c_str(), INVALID_CONFIG_DATA_LEN);
            m_invalidUsgS[INVALID_CONFIG_DATA_LEN] = 0;
         }
         if (m_maxUsageLogfileSize != stmpval) {
            m_maxUsageLogfileSize = stmpval;
            m_changedMask |= MaskBit_Maximum_Usage_Logfile_Size;
         } 
      } else if (name == "Debug_Level") {
         stmpval = atoi(value.c_str());
         if (stmpval < (long) DebugLevelMin_g || stmpval > (long) DebugLevelMax_g) {
            stmpval = DebugLevel_g; 
         }
         if (stmpval != m_debugLevel) {
            m_debugLevel = stmpval;
            m_changedMask |= MaskBit_Debug_Level;
         }
      } else {
         //discard unrecognized config data
      }
   }
   
   //Notify registered objects that config data has changed
   if (m_changedMask) {
      for (std::vector<MLMDynamicConfig *>::iterator curr = m_changeCbkRegistry.begin(); curr != m_changeCbkRegistry.end(); curr++) {
         (*curr)->ConfigChanged(m_changedMask);
      }
   }

   return (m_changedMask);
}

void MLMConfiguration::RegisterForConfigChanges(MLMDynamicConfig * ptr)
{
   if (ptr) m_changeCbkRegistry.push_back(ptr);
   return;
}

bool MLMConfiguration::CheckUpdates(void)
{
   return (m_changedMask ? true:false);
}

std::string MLMConfiguration::Trim(std::string const& src, char const* delims) 
{
   std::string result(src);

   std::string::size_type index = result.find_last_not_of(delims);
  if (index != std::string::npos)
    result.erase(++index);

  index = result.find_first_not_of(delims);
  if (index != std::string::npos) result.erase(0, index);
  else result.erase();

  return (result);
}

unsigned int MLMConfiguration::ValidateDir(std::string dirname, std::string & valid)
{
   DIR * d_entry = 0;
   unsigned int saveErrno = 0;

   if ((d_entry = opendir(dirname.c_str())) == 0) {
      //could not open directory
      
      saveErrno = errno;
      std::string errmsg = (saveErrno < sys_nerr? sys_errlist[saveErrno] : "Unknown Error");
      std::string cmdname = "mkdir ";
      cmdname += dirname;

      switch(saveErrno) {
         case ENOENT: //dir does not exist, try to create it
            if (system(cmdname.c_str())) {
               //could not create dir, set default
               cerr << "Directory " << dirname.c_str() << " could not be created, errno=" << saveErrno << ": " << errmsg.c_str() << endl;
               cerr << "Setting Log_Directory to default " <<
                  MLMConstants::LogDir_g << endl;
               dirname = MLMConstants::LogDir_g;
            } else {
               //dir created, validate newly created dir
               std::string valid_dir;
               if (ValidateDir(dirname, valid_dir) == ENOENT) {
                  dirname = MLMConstants::LogDir_g;
               } else {
                  dirname = valid_dir;
               }
            }
            break;

         case ENOTDIR: //not a directory, set default
         case ENAMETOOLONG: //dir name too long, set default
         case ELOOP: //dir name is complex, too many symbolic links, set default
         case EACCES://no search permission for component of dir OR no read permission for dir, set default
            cerr << "Directory " << dirname.c_str() << " could not be opened, errno=" << saveErrno << ": " << errmsg.c_str() << endl;
            cerr << "Setting Log_Directory to default " <<
               MLMConstants::LogDir_g << endl;
            dirname = MLMConstants::LogDir_g;
            break;

         case EMFILE: //OPEN_MAX FDs currently open, bail out
         case ENFILE: //too many files open, bail out
            cerr << "Cannot open Log Directory, too many resources in use, errno=" << saveErrno << ", aborting with exit code=" << MLM_EXIT_CODE_TOO_MANY_FILES_OPEN << endl;
            exit(MLM_EXIT_CODE_TOO_MANY_FILES_OPEN);
         default:
            break;
      }
   } else {
      //directory exists and is readable, check for write/execute permissions
      closedir(d_entry);
      std::fstream testStrm;
      std::string testfile = dirname;
      testfile += "/";
      testfile += ".test7378347474";
      testStrm.open(testfile.c_str(), std::ios::out|std::ios::app);
      if (!testStrm.good()) {
         cerr << "Directory "<< dirname.c_str() << " does not have permissions to create a file" << endl;
         cerr << "Setting Log_Directory to default " <<
            MLMConstants::LogDir_g << endl;
         dirname = MLMConstants::LogDir_g;
      }
      testStrm.close();
      remove(testfile.c_str());
   }

   valid = dirname;
   return (saveErrno);
}
