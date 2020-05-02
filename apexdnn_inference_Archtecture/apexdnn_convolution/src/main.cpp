/*****************************************************************************
 *
 * NXP Confidential Proprietary
 *
 * Copyright (c) 2017 NXP Semiconductor;
 * All Rights Reserved
 *
 *****************************************************************************
 *
 * THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <stdio.h>
#include <vector>

#include "apex.h"
#include "airunner_convlayer.hpp"

int main()
{
   int lRetVal = 0;

#ifndef APEX2_EMULATE
   APEX_Init();
#endif

   ////////////////////////
   // layer unit test cases
   ////////////////////////
   std::vector<std::pair<int, std::string>> lResults;

   lResults.emplace_back(conv_layer_test(1), "conv layer");
   std::cout << "=========================\n";
   std::cout << " SUMMARY \n";
   std::cout << "=========================\n";
   for(auto &result : lResults)
   {
     if(result.first == 0)
     {
       std::cout << "[ SUCCESS ] ";
     }
     else
     {
       std::cout << "[=FAILURE=] ";
       lRetVal = 1;
     }
     std::cout << result.second << std::endl;
   }
   if(lRetVal == 0)
   {
      printf("=========================\n");
      printf(" AIRUNNER CONVLAYER PROCESSING SUCCESS \n");
      printf("=========================\n");
   }
   else
   {
      printf("========================\n");
      printf(" Unit Test Cases FAILED \n");
      printf("========================\n");
   }
   return 0;
}
