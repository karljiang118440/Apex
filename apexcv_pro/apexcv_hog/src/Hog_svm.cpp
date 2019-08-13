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

#include <common_helpers.h>
#include <string>

//********************************************************************

#ifdef INPUT_PATH_REL
#define INPUT_ROOT __FULL_DIR__ XSTR(INPUT_PATH_REL)/data/common/
#else
#define INPUT_ROOT "data/common/"
#endif
#ifdef OUTPUT_PATH_REL
#define OUTPUT_ROOT __FULL_DIR__ XSTR(OUTPUT_PATH_REL)/data/output/
#else
#define OUTPUT_ROOT "data/output/"
#endif



#include <apexcv_pro_hog.h>
#include <gdc_pd_svm_function.h>
#define HOG_SVM

#include <opencv2/opencv.hpp>  
#include <umat.hpp>
//#include <APU_HISTOGRAM.hpp>
#include "apex.h"
#include "common_time_measure.h"
#include <oal.h>
#include <stdint.h>
#include <umat.hpp>
#include <stdio.h>
using namespace cv;
using namespace apexcv;



/****************************************************************************
* Main function
****************************************************************************/
int main(int argc, char** argv)
{

#ifdef HOG_SVM

APEX_Init();
apexcv::Hog apex_hog;


void APEX_Hog(vsdk::UMat &Src,vsdk::UMat &Dst,vsdk::UMat &SvmModel);


//vsdk::UMat lSceneY_8bit_umat;
//readImageFiler("data/apps/pedestrian_detection/gdc_pd_640x480.y",lSceneY_8bit_umat);



 
vsdk::UMat lSceneY_8bit_umat = cv::imread(INPUT_ROOT"in_grey_640x480.png", 0).getUMat(cv::ACCESS_READ); 




vsdk::UMat lSceneRsY_8bit_umat=lSceneY_8bit_umat;


vsdk::UMat lSceneRsY_24bit_umat = cv::imread(INPUT_ROOT"in_color_640x480.png", 1).getUMat(cv::ACCESS_READ); 

#if 0
vsdk::UMat lSceneRsY_24bit_umat=(lSceneY_8bit_umat.rows,lSceneRsY_8bit_umat,VSDK_CV_8UC3,vsdk::USAGE_DDR0);

APEX_InsertChannel(lSceneRsY_8bit_umat,lSceneRsY_24bit_umat,1,ApexKernel_0);
APEX_InsertChannel(lSceneRsY_8bit_umat,lSceneRsY_24bit_umat,2,ApexKernel_0);
APEX_InsertChannel(lSceneRsY_8bit_umat,lSceneRsY_24bit_umat,3,ApexKernel_0);

#endif


vsdk::UMat SVMmodel_64bit_fcmat(1,gdc_pd_svm_function_bin_len,VSDK_CV_64FC1,vsdk::USAGE_DDR0);
double *pSvmDouble=(double*)SVMmodel_64bit_fcmat.getMat(cv::ACCESS_RW|OAL_USAGE_CACHED).data;
memcpy(pSvmDouble,gdc_pd_svm_function_bin,gdc_pd_svm_function_bin_len);


vsdk::UMat HogScores_16bit_scmt;
APEX_Hog(lSceneRsY_8bit_umat,HogScores_16bit_scmt,SVMmodel_64bit_fcmat);

vsdk::Mat lHogScores_mat = HogScores_16bit_scmt.getMat(vsdk::ACESS_READ|OAL_USAGE_CACHED);
const int16_t* const cpcHogScoresS16=(const int16_t*const)(((uintptr_t)lHogScores_mat.data));




// hog detect pedestrains 

uint32_t mSceneResizesHogCounters[20];
memset(mSceneResizesHogCounters,0,sizeof(mSceneResizesHogCounters));


//if Hog detect result is pedestrian than drawn opencv box
ROI pedestrianRoi;
const int hog_threshold=800;
cv::Mat imgSrc=lSceneRsY_24bit_umat.getMat(cv::ACCESS_RW|OAL_USAGE_CACHED);
for(int32_t y=0;y<HogScores_16bit_scmt.rows;++y){
  for(int32_t x=0;x<HogScores_16bit_scmt.cols;++x){
    const int16_t cScore=
        cpcHogScoresS16[y*(HogScores_16bit_scmt.step[0]/HogScores_16bit_scmt.elemSize())+x];
      if(cScore>hog_threshold){
        pedestrianRoi.x=x*4;
        pedestrianRoi.y=y*4;
        pedestrianRoi.width=64;
        pedestrianRoi.height=128;
        memset(pedestrianRoi.mResolutionDetectionCounters,0,sizeof(pedestrianRoi.mResolutionDetectionCounters));
        ++(pedestrianRoi.mResolutionDetectionCounters[0]);
        ++mSceneResizesHogCounters[0];
        rectangle(imgSrc,Point(pedestrianRoi.x,pedestrianRoi.y),
        Point(pedestrianRoi.x+pedestrianRoi.width,pedestrianRoi.y+pedestrianRoi.height),
        Scalar(0,0,255),2);
      }
  }
}

/********
(1)
*********/



APEX_Hog(){
  
}










  #else





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










  #endif























}
