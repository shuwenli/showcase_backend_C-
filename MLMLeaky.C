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
#include <iostream.h>
#include <errno.h>
#include <time.h>

int main(int argc, char * argv[] )
{
   int kb = 0;
   cout << "KB to leak every minute:";
   cin >> kb;

   double leakBytesPerSec = ((double)kb * 1024)/60;
   
   while(true)
   {  
      unsigned short sec = 0;
      for(; sec<60; ++sec) {
         char * ptr = new char[leakBytesPerSec];
         ptr = 0;

         //idle for 1 sec
         int rv = 0;
         struct timeval tv;
         tv.tv_sec = 1;
         tv.tv_usec = 0;
         while ((rv = select(0, 0, 0, 0, &tv)) == EINTR);
      }
   }

   //will never get here
   return 0;
}
