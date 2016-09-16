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
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "MLMConstants.h"
#include "MLMPIDScanner.h"
#include "MLMLogger.h"

extern char * sys_errlist[];
extern int sys_nerr;

MLMPIDScanner::MLMPIDScanner() : m_dir(0), m_log(MLMLogger::Instance())
{
   m_buf[0] = 0;
   Open();
   MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MEM, "Created MLMPIDScanner object of size %d bytes", sizeof(*this));
}

MLMPIDScanner::~MLMPIDScanner()
{
   MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MEM, "Destroyed MLMPIDScanner object of size %d bytes", sizeof(*this));
   Close();
}

short int MLMPIDScanner::Open(void)
{
   if (m_dir) return (MLM_ERROR); //already open

   if ((m_dir = opendir(MLMConstants::ProcDir_g)) == 0) {
      int saveErrno = errno;
      std::string errmsg = (saveErrno < sys_nerr? sys_errlist[saveErrno] : "Unknown Error");

      MLM_LOG_ERROR2(m_log, "opendir() failed: errno=%d, %s", saveErrno, errmsg.c_str());
      return (MLM_ERROR);      //cannot read proc
   }

   return (0);
}

long MLMPIDScanner::Next(void)
{
   if (m_dir == 0) return (MLM_ERROR);  //not open

   struct dirent *p_dirEntry = 0;
   while ((p_dirEntry = readdir(m_dir)) != 0) {
      if (p_dirEntry->d_name[0] == '.') continue; /* skip entries for . and .. */
      //MLM_LOG_DEBUG1(m_log, MLM_DEBUG_MAX, "MLMPIDScanner::Next() pid=%s", p_dirEntry->d_name);
      strcpy(m_buf, p_dirEntry->d_name); 
      return (atol(m_buf));
   }
   return (MLM_EOF);
}

short int MLMPIDScanner::Close(void)
{
   if (m_dir == 0) return (MLM_ERROR); //not open
   closedir(m_dir);
   return (0);
}

