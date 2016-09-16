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
#include <stdlib.h>
#include <math.h>

#define MAX_SLOTS 12000


int main (int argc, char * argv[])
{
   int data[MAX_SLOTS];
   int numTolSlots;

   if (argc < 2) {
      cout << "Usage: " << argv[0] << " <num_tolerance_slots> [skip_value] [{0|1}]" << endl;
      cout << "       where the 0/1 value represents a formatted display of output" << endl;
      return 0;
   }

   bool dispFl = false;
   if (argc > 3) dispFl = true;

   numTolSlots = atoi(argv[1]);
   if (numTolSlots>MAX_SLOTS) numTolSlots = MAX_SLOTS;
   if (!dispFl) cout << "numTolSlots=" << numTolSlots << endl;

   int skip = 0;
   if (argc > 2) skip = atoi(argv[2]); 
   if (!dispFl) cout << "skip=" << skip << endl;

   double currAvg = 0, prevAvg = 0;

   bool exitFl = false;
   int loopnum = 1;

   while (true) {
      int currNumTolSlots = numTolSlots;


      prevAvg = currAvg;
      currAvg = 0;

      int i=0;
      int dummy = 0;

      for (i=0; i<currNumTolSlots; ++i) {
         //cout << i+1 << ".)";
         for (int j=0; j<skip; j++) {
            cin >> dummy;
            if (dummy == 0) {
               exitFl = true;
               break;
            }
         }
         cin >> data[i];
         if (data[i] == 0) {
            exitFl = true;
            break;
         }
      }

      if (exitFl) { currNumTolSlots = i; }
      if (currNumTolSlots == 0) break;

      if (!dispFl) cout << "Tolerence Interval: " << loopnum << endl;
      if (!dispFl) cout << "   Number of Slots: " << currNumTolSlots << endl;
      ++loopnum;

      int isLeaky = 0;

      //Check for progressive increase
      bool isDidDecrease = false;
      bool isDidIncrease = false;
      for (unsigned int i=1; i<currNumTolSlots; ++i) {
         if (data[i] < data[i-1]) isDidDecrease = true;
         if (data[i] > data[i-1]) isDidIncrease = true;
         currAvg += (double) data[i];
      }
      if (isDidIncrease && !isDidDecrease) isLeaky = 1;
      if (!dispFl) cout << "   isLeaky=" << isLeaky << endl;

      //calculate the average memory usage for the current tolerance interval
      currAvg /= currNumTolSlots;
      
      //compare current average with previous average
      //if previous average was zero, shouldn't compare averages
      if (prevAvg) {
         if (currAvg > prevAvg) {
            isLeaky = 2;
         }
      }

      if (!dispFl) cout << "   isLeaky=" << isLeaky << " currAvg=" << currAvg << " prevAvg=" << prevAvg << endl;

      double sigX = 0;
      double sigY = 0;
      double sigXY = 0;
      double sigXX = 0;
      double sigYY = 0;

      //cout << endl << "X Y sigX sigY sigXY sigXX sigYY" << endl;
      for (i=0; i<currNumTolSlots; ++i) {
         sigX += ((double)i+1);
         sigY += ((double)data[i]);
         sigXY += ((double)i+1)*((double)data[i]);
         sigXX += ((double)i*(double)i)+(2*(double)i)+1;
         sigYY += ((double)data[i])*((double)data[i]);
         //cout << (i+1) << " " << data[i] << " " << sigX << " " << sigY << " " << sigXY << " " << sigXX << " " << sigYY << endl;
      }

      double r_num = (currNumTolSlots*sigXY) - (sigX*sigY);
      //cout << "   r_num=" << r_num << endl;

      double r_den2 = ((currNumTolSlots*sigXX) - (sigX*sigX))*((currNumTolSlots*sigYY) - (sigY*sigY));
      //cout << "   r_den2=" << r_den2 << endl;

      double r_den = sqrt(r_den2);
      //cout << "   r_den=" << r_den << endl;

      double r = 0;
      if (r_den) r = r_num/r_den;
      if (!dispFl) cout << "   Correlation Coefficient is:" << r << endl; 
      else cout << (loopnum-1) << " " << currNumTolSlots << " " << currAvg << " " << r << endl;

      if(exitFl) break;

   }

   return 0;
}
