/*****************************************************************************
*
* NXP Confidential Proprietary
*
* Copyright (c) 2017 NXP
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

/****************************************************************************
* Includes
****************************************************************************/

#ifndef APEX2_EMULATE
#include "apex.h"
#endif

#include <apexcv_pro_remap.h>
#include <common_helpers.h>
#include <string>

/****************************************************************************
* Main function
****************************************************************************/
int main(int argc, char** argv)
{
  std::string helpMsg = std::string("Apexcv demo, demonstrating remapping with the ApexCV-Pro library.\n\tUsage: ") +
                        COMMON_ExtractProgramName(argv[0]);
  int idxHelp = COMMON_HelpMessage(argc, argv, helpMsg.c_str());
  if(idxHelp > 0)
  {
    //found help in application arguments thus exiting application
    return -1;
  }

  // This is needed only for the Target Compiler
  // HW and resources init
  APEX_Init();

  const int clSrcWidth  = 1024;
  const int clSrcHeight = 512;

  // Allocate the input and output buffers
  vsdk::UMat lInput0(clSrcHeight, clSrcWidth, VSDK_CV_8UC1);
  vsdk::UMat lOutput0(clSrcHeight, clSrcWidth, VSDK_CV_8UC1);

  float lTransform[clSrcWidth * clSrcHeight * 2];
  // Create an horizontal mirroring mapping
  for(int x = 0; x < clSrcWidth; x++)
  {
    for(int y = 0; y < clSrcHeight; y++)
    {
      int lOffset             = 2 * ((y * clSrcWidth) + x);
      lTransform[lOffset + 0] = clSrcWidth - 1 - x; //x
      lTransform[lOffset + 1] = y;                  //y
    }
  }

  // Control output
  printf("Data on: \n");
  printf("   Input             (%dx%d bytes) at %p\n", clSrcWidth, clSrcHeight, lInput0.u->handle);
  printf("   Output            (%dx%d words) at %p\n", clSrcWidth, clSrcHeight, lOutput0.u->handle);

  int lRetVal = 0;

  // Create a new add process for the Apex core.
  apexcv::Remap myRemap;

  // Initialize the process, this is the mandatory first call to apexcv object
  lRetVal |= myRemap.Initialize(lTransform, clSrcWidth, clSrcHeight, clSrcWidth, apexcv::INTER_TYPE::INTER_LINEAR,
                                apexcv::BORDER_TYPE::BORDER_CONSTANT, 0);
  if(lRetVal)
  {
    printf("Error on Initialize: %d \n", lRetVal);
    goto END;
  }
  printf("Initialize Done \n");

  // Compute the results, the output buffer is updated after Process executes
  lRetVal |= myRemap.Process(lInput0, lOutput0);
  if(lRetVal)
  {
    printf("Error on Process: %d \n", lRetVal);
    goto END;
  }
  printf("Process Done \n");

  // Display few results
  printf("Display Source top left corner\n");
  { // isolate the virtual address mapping of the UMat object
    // fillup the input buffer
    vsdk::Mat mat = lInput0.getMat(OAL_USAGE_CACHED | vsdk::ACCESS_RW);
    if(!mat.empty())
    {
      uint8_t* pData = (uint8_t*)mat.data;
      for(int y = 0; y < 2; y++)
      {
        for(int x = 0; x < 4; x++)
        {
          printf("%d, ", pData[(y * clSrcWidth * 2) + x]);
        }
        printf("\n");
      }
    }
  } // release the virtual address mapping of the UMat object

  // Display few results
  printf("Display results top right corner (displayed in revers order, so it should look like the source)\n");
  { // isolate the virtual address mapping of the UMat object
    // fillup the input buffer
    vsdk::Mat mat = lOutput0.getMat(OAL_USAGE_CACHED | vsdk::ACCESS_RW);
    if(!mat.empty())
    {
      uint8_t* pData = (uint8_t*)mat.data;
      for(int y = 0; y < 2; y++)
      {
        for(int x = clSrcWidth - 1; x > clSrcWidth - 1 - 4; x--)
        {
          printf("%d, ", pData[(y * clSrcWidth * 2) + x]);
        }
        printf("\n");
      }
    }
  } // release the virtual address mapping of the UMat object

END:

  if(0 != lRetVal)
  {
    printf("Program Ended Error 0x%X [ERROR]\n", lRetVal);
  }
  else
  {
    printf("Program Ended [SUCCESS]\n");
  }

  return lRetVal;
  ;
}
