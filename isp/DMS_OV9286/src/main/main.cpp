/*****************************************************************************
*
* Freescale Confidential Proprietary
*
* Copyright (c) 2016 Freescale Semiconductor;
* All Rights Reserved
*
*****************************************************************************
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
****************************************************************************/

#ifndef __STANDALONE__
  #include <signal.h>
#endif // #ifdef __STANDALONE__
#include <string.h>

#ifdef __STANDALONE__
  #include "frame_output_dcu.h"
  #define CHNL_CNT io::IO_DATA_CH3
#else // #ifdef __STANDALONE__
  #include "frame_output_v234fb.h"
  #define CHNL_CNT io::IO_DATA_CH3
#endif // else from #ifdef __STANDALONE__  

#include "sdi.hpp"
#include "ar0144_max9286_mipi_c.h"
#include "gamma_table.h"

#include "vdb_log.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

//***************************************************************************
// constants
//***************************************************************************

// Possible to set input resolution (must be supported by the DCU)
#define WIDTH           1280 ///< width of DDR buffer in pixels
#define HEIGHT          720 ///< height of DDR buffer in pixels
#define DDR_BUFFER_CNT  3    ///< number of DDR buffers per ISP stream


#define  SEQ_IPUS_TYPE        1
#define  SEQ_IPUV_TYPE        0

#define GAMMA_ENGINE 			2

#define LUTA_REG 0x78
#define LUTD_REG 0x7c

#define  SEQ_GET_DIRECT_VALUE    1
#define  SEQ_GET_INDIRECT_VALUE  0

#define OUTPUT_ROOT "imgsave/"

Mat rgbImg;				//Image container in RGB888 format for display 
Mat grayImg;			//Image container in grayscale format for most of the image processing


//***************************************************************************

#define FRM_TIME_MSR 60 ///< number of frames to measure the time and fps

//***************************************************************************
// macros
//***************************************************************************

#ifdef __STANDALONE__
//extern SEQ_Buf_t  producerless_buffer_1;
extern "C" {
  unsigned long get_uptime_microS(void);
}

#define GETTIME(time)   (*(time)=get_uptime_microS())
#else // ifdef __STANDALONE__
#define GETTIME(time) \
  { \
  struct timeval lTime; gettimeofday(&lTime,0); \
  *time=(lTime.tv_sec*1000000+lTime.tv_usec);   \
  }
#endif // else from #ifdef __STANDALONE__

//***************************************************************************
// types
//***************************************************************************
struct AppContext
{
  sdi_grabber *mpGrabber;      ///< pointer to grabber instance
  sdi_FdmaIO  *mpFdma;         ///< pointer to fdma object
  
  // ** event counters and flags **
  bool     mError;            ///< to signal ISP problems
  uint32_t mFrmDoneCnt;       ///< number of frames done events
  uint32_t mFrmCnt;           ///< number of frames grabbed by the app  
}; // struct AppContext

/************************************************************************/
/**Gamma correction array ***********************************************/
static void GammaCorrect(void);

/************************************************************************/
/** User defined call-back function for Sequencer event handling.
  * 
  * \param  aEventType defines Sequencer event type
  * \param  apUserVal  pointer to any user defined object 
  ************************************************************************/
static void SeqEventCallBack(uint32_t aEventType, void* apUserVal);

/************************************************************************/
/** Prepare everything before executing the main functionality .
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, <0 otherwise 
  ************************************************************************/
static int32_t Prepare(AppContext &arContext);

/************************************************************************/
/** Initial setup of application context.
  * 
  * \param arContext structure capturing the context of the application
  ************************************************************************/
static void ContextInit(AppContext &arContext);

/************************************************************************/
/** Prepares required libraries.
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, != 0 otherwise 
  ************************************************************************/
static int32_t LibsPrepare(AppContext &arContext);

/************************************************************************/
/** Prepares DDR buffers.
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, != 0 otherwise 
  ************************************************************************/
static int32_t DdrBuffersPrepare(AppContext &arContext);

/************************************************************************/
/** Execute main functionality of the application.
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, <0 otherwise 
  ************************************************************************/
static int32_t Run(AppContext &arContext);

/************************************************************************/
/** Cleanup all resources before application end.
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, <0 otherwise 
  ************************************************************************/
static int32_t Cleanup(AppContext &arContext);

#ifndef __STANDALONE__
/************************************************************************/
/** SIGINT handler.
  * 
  * \param  aSigNo 
  ************************************************************************/
static void SigintHandler(int aSigNo);

/************************************************************************/
/** SIGINT handler.
  * 
  * \param  aSigNo 
  * 
  * \return SEQ_LIB_SUCCESS if all ok
  *         SEQ_LIB_FAILURE if failed
  ************************************************************************/
static int32_t SigintSetup(void);

//***************************************************************************

static bool sStop = false; ///< to signal Ctrl+c from command line

#endif // #ifndef __STANDALONE__

int main(int, char **)
{
  int lRet = 0;

  AppContext lContext;
     
  //*** process command line parameters ***
  printf("\n**************************************************************\n"
         "** AR0144 ISP processing demo\n"
         "**************************************************************\n\n");
#ifndef __STANDALONE__  
  fflush(stdout);  
  sleep(1);
#endif // ifndef __STANDALONE__
  
  if(Prepare(lContext) == 0)
  {
    if(Run(lContext) != 0)
    {
      printf("Demo execution failed.\n");
      lRet = -1;
    } // if Run() failed
  } // if Prepare() ok
  else
  {
    printf("Demo failed in preparation phase.\n");
    lRet = -1;
  }// else from if Prepare() ok
  
  if(Cleanup(lContext) != 0)
  {
    printf("Demo failed in cleanup phase.\n");
    lRet = -1;
  } // if cleanup failed

  return lRet;
} // main()

//***************************************************************************

static int32_t Prepare(AppContext &arContext)
{  
  // init app context
  ContextInit(arContext);
  
  // enable LIBS
  if(LibsPrepare(arContext) != 0)
  {
    printf("Failed to prepare libraries.\n");
    return -1;
  } // if failed to configure decoder
  // enable OAL
  
  if(DdrBuffersPrepare(arContext) != 0)
  {
    printf("Failed to prepare DDR buffers.\n");
    return -1;
  } // if fialed to prepare DDR buffers  


  // *** prestart grabber ***
  if(arContext.mpGrabber->PreStart() != LIB_SUCCESS)
  {
    printf("Failed to prestart the grabber.\n");
    return -1;
  } // if PreStart() failed
  
  if(arContext.mpGrabber->SeqEventCallBackInstall
  (
    &SeqEventCallBack, 
    &arContext
  ) != LIB_SUCCESS)
  {
    printf("Failed to install Sequencer event callback.\n");
    return -1;
  } // if callback setup failed
   
  return 0;
} // Prepare()

//***************************************************************************

static void ContextInit(AppContext &arContext)
{
  arContext.mpGrabber   = NULL;      
  arContext.mpFdma      = NULL; 
  arContext.mError      = false;
  arContext.mFrmCnt     = 0;
  arContext.mFrmDoneCnt = 0;
} // ContextInit()

//***************************************************************************

static int32_t LibsPrepare(AppContext &arContext)
{
  // *** Initialize SDI ***
  if(sdi::Initialize(0) != LIB_SUCCESS)
  {
    printf("Failed to initialzie SDI.\n");
    return -1;
  } // if failed to initialize SDI
  
  // create grabber
  arContext.mpGrabber = new(sdi_grabber);  
  if(arContext.mpGrabber == NULL)
  {    
    printf("Failed to create sdi grabber.\n");
    return -1;
  } // if failed to create grabber
  
  if(arContext.mpGrabber->ProcessSet(gpGraph, &gGraphMetadata) != LIB_SUCCESS)
  {
    printf("Failed to set ISP graph to grabber.\n");
    return -1;
  } // if ISP graph not set
  
  // get IOs
  arContext.mpFdma = (sdi_FdmaIO*)arContext.mpGrabber->IoGet(SEQ_OTHRIX_FDMA);    
  if(arContext.mpFdma == NULL)
  {
    printf("Failed to get FDMA object.\n");
    return -1;
  } // if no FDMA object
  
  return 0;
} // LibsPrepare(AppContext &arContext)

//***************************************************************************

static int32_t DdrBuffersPrepare(AppContext &arContext)
{
  // *** RGB full buffer array ***
  // modify DDR frame geometry to fit display output
  SDI_ImageDescriptor lFrmDesc;
  lFrmDesc = SDI_ImageDescriptor(WIDTH, HEIGHT, RGB888);
  if(arContext.mpFdma->DdrBufferDescSet(FDMA_IX_FDMA_0, lFrmDesc) 
     != LIB_SUCCESS)
  {
    printf("Failed to set image descriptor.\n");
    return -1;
  } // if descriptor setup has failed   
  
  lFrmDesc = SDI_ImageDescriptor(WIDTH, HEIGHT, RGB888);//GS8 -> RGB888
  if(arContext.mpFdma->DdrBufferDescSet(FDMA_IX_FDMA_0, lFrmDesc) 
     != LIB_SUCCESS)
  {
    printf("Failed to set image descriptor.\n");
    return -1;
  } // if descriptor setup has failed     
  
  //*** allocate DDR buffers ***
  if(arContext.mpFdma->DdrBuffersAlloc(DDR_BUFFER_CNT)
     != LIB_SUCCESS)
  {
    printf("Failed to allocate DDR buffers.\n");
    return -1;
  } // if buffer alloc has failed
  
  return 0;
} // DdrBuffersPrepare(AppContext &arContext)

//***************************************************************************

static int32_t Run(AppContext &arContext)
{  
  //*** Init DCU Output ***
#ifdef __STANDALONE__
  io::FrameOutputDCU lDcuOutput(
                      WIDTH, 
                      HEIGHT, 
                      io::IO_DATA_DEPTH_08, 
                      CHNL_CNT);
#else // #ifdef __STANDALONE__
  // setup Ctrl+C handler
  if(SigintSetup() != SEQ_LIB_SUCCESS) 
  {
    VDB_LOG_ERROR("Failed to register Ctrl+C signal handler.");
    return -1;
  }
  
  printf("Press Ctrl+C to terminate the demo.\n");
  
  io::FrameOutputV234Fb lDcuOutput(
                          WIDTH, 
                          HEIGHT, 
                          io::IO_DATA_DEPTH_08, 
                          CHNL_CNT);
#endif // else from #ifdef __STANDALONE__
  
  unsigned long lTimeStart = 0, lTimeEnd = 0, lTimeDiff = 0;
//  GammaCorrect();  
  // *** start grabbing ***
  GETTIME(&lTimeStart);
  if(arContext.mpGrabber->Start() != LIB_SUCCESS)
  {
    printf("Failed to start the grabber.\n");
    return -1;
  } // if Start() failed
  
  SDI_Frame lFrame[FDMA_IX_END];
  // *** grabbing/processing loop ***  
  for(;;)
  {
    lFrame[FDMA_IX_FDMA_0] = arContext.mpGrabber->FramePop(FDMA_IX_FDMA_0);
 /*   lFrame[FDMA_IX_FDMA_0] = arContext.mpGrabber->FramePop(FDMA_IX_FDMA_0);*/

    if(lFrame[FDMA_IX_FDMA_0].mUMat.empty())
    {
      printf("Failed to grab image number %u\n", arContext.mFrmCnt);
      arContext.mError = true;
      break;
    } // if pop failed	
 /*   if(lFrame[FDMA_IX_FDMA_0].mUMat.empty())
    {
      printf("Failed to grab image number %u\n", arContext.mFrmCnt);
      arContext.mError = true;
      break;
    } // if pop failed*/

    arContext.mFrmCnt++;
   
    lDcuOutput.PutFrame(lFrame[FDMA_IX_FDMA_0].mUMat);

    if(arContext.mpGrabber->FramePush(lFrame[FDMA_IX_FDMA_0]) != LIB_SUCCESS)
    {
      printf("Failed to push image number %u\n", arContext.mFrmCnt);
      arContext.mError = true;
      break;
    } // if push failed    
 /*   if(arContext.mpGrabber->FramePush(lFrame[FDMA_IX_FDMA_0]) != LIB_SUCCESS)
    {
      printf("Failed to push image number %u\n", arContext.mFrmCnt);
      arContext.mError = true;
      break;
    } // if push failed  */	
    
    if((arContext.mFrmCnt % FRM_TIME_MSR) == 0)
    {
      GETTIME(&lTimeEnd);
      lTimeDiff  = lTimeEnd - lTimeStart;
      lTimeStart = lTimeEnd;
	  
      //rgbImg = (vsdk::Mat)lFrame[FDMA_IX_FastDMA_RGB_Out].mUMat.getMat(ACCESS_WRITE | OAL_USAGE_CACHED);
      //grayImg = (vsdk::Mat)lFrame[FDMA_IX_FastDMA_GREY_Out].mUMat.getMat(ACCESS_WRITE | OAL_USAGE_CACHED);

      //cv::imwrite(string(OUTPUT_ROOT)+"framePopRGB"+to_string(arContext.mFrmCnt)+".jpg", rgbImg);
      //cv::imwrite(string(OUTPUT_ROOT)+"framePopGS"+to_string(arContext.mFrmCnt)+".jpg", grayImg);		  
      
      //cv::imwrite("framePopRGB"+to_string(arContext.mFrmCnt)+".jpg", rgbImg);
      //cv::imwrite("framePopGS"+to_string(arContext.mFrmCnt)+".jpg", grayImg);		  
      
      printf("%u frames took %lu usec (%5.2ffps)\n",
             FRM_TIME_MSR,
             lTimeDiff,
             (FRM_TIME_MSR*1000000.0)/((float)lTimeDiff));
    }// if time should be measured
    
#ifndef __STANDALONE__  
    if(sStop)
    {
      break; // break if Ctrl+C pressed
    } // if Ctrl+C
#endif //#ifndef __STANDALONE__  
  } // for ever
    
  return 0;
} // Run()

//***************************************************************************

static int32_t Cleanup(AppContext &arContext)
{
  int32_t lRet = 0;
  
  if(arContext.mpGrabber != NULL)
  {
    if(arContext.mpGrabber->Stop())
    {
      printf("Failed to stop the grabber.\n");
      lRet = -1;
    } // if grabber stop failed
    
    if(arContext.mpGrabber->Release())
    {
      printf("Failed to release grabber resources.\n");
      lRet = -1;
    } // if grabber resources not released
    
    delete(arContext.mpGrabber);
    arContext.mpGrabber = NULL;
  } // if grabber exists
  
#ifdef __STANDALONE__
  for(;;);  // *** don't return *** 
#endif // #ifdef __STANDALONE__    
  
  if(sdi::Close(0) != LIB_SUCCESS)
  {
    printf("Failed to terminate use of SDI.\n");
    lRet = -1;
  } // if SDI use termination failed
  
  return lRet;
} // Cleanup()

/************************************************************************/
//#if 0
static void GammaCorrect(void)
{
	//uint32_t *GammaArray[4096] = Ar0144_Gamma_Table;

   seq_setReg(SEQ_IPUS_TYPE, GAMMA_ENGINE, 1, LUTA_REG, 0); // LUTA (IPUS)
     {
       uint16_t lutcnt = 0;
       while (lutcnt < 2048) 
	 {
            uint32_t val;
            val = (GammaArray[lutcnt*2] | (GammaArray[lutcnt*2+1]<<16));
            lutcnt++;
            seq_setReg(SEQ_IPUS_TYPE, GAMMA_ENGINE, 1, LUTD_REG, val);
         }
     } 

}
//#endif
//***************************************************************************
//***************************************************************************

static void SeqEventCallBack(uint32_t aEventType, void* apUserVal)
{
  AppContext *lpAppContext = (AppContext*)apUserVal;
  
  if(lpAppContext)
  {  
    if(aEventType == SEQ_MSG_TYPE_FRAMEDONE)
    {
     // printf("Frame done message arrived #%u.\n", 
            //lpAppContext->mFrmDoneCnt++);
    } // if frame done arrived
  } // if user pointer is NULL
} // SeqEventCallBack()

//***************************************************************************

#ifndef __STANDALONE__
static void SigintHandler(int)
{
  sStop = true;
} // SigintHandler()

//***************************************************************************

static int32_t SigintSetup()
{
  int32_t lRet = SEQ_LIB_SUCCESS;
  
  // prepare internal signal handler
  struct sigaction lSa;
  memset(&lSa, 0, sizeof(lSa));
  lSa.sa_handler = SigintHandler;
  
  if( sigaction(SIGINT, &lSa, NULL) != 0)
  {
    VDB_LOG_ERROR("Failed to register signal handler.\n");
    lRet = SEQ_LIB_FAILURE;
  } // if signal not registered
  
  return lRet;
} // SigintSetup()

//***************************************************************************
#endif // #ifndef __STANDALONE__
