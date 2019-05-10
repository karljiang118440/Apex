/*****************************************************************************
*
* Copyright (c) 2018 NXP
*
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
  #include <termios.h>
#endif // #ifdef __STANDALONE__
#include <string.h>

#ifdef __STANDALONE__
  #include "frame_output_dcu.h"
  #include "uartlinflex_driver.h"
#else // #ifdef __STANDALONE__
  #include "frame_output_v234fb.h"
#endif // else from #ifdef __STANDALONE__
#define CHNL_CNTA io::IO_DATA_CH3

#include "sdi.hpp"
#include "isp_cam_sony.h"

#include "vdb_log.h"

#include "../include/vignetting.h"      ///< vignetting table
//#include "sony_dualexp_c.h"             ///< graph related defines
#include "mipi_simple_c.h"              ///< graph related defines
#include "mipi_simple_1_c.h"
//#include "iofiles_sony_dualexp.h"       ///< parameters from graph
#include "iofiles_mipi_simple.h"       ///< parameters from graph
#include "iofiles_mipi_simple_1.h"
#include <cmath>

//***************************************************************************
// macros
//***************************************************************************
#define WIDTH                     1280            ///< width of DDR buffer in pixels
#define HEIGHT                    720             ///< height of DDR buffer in pixels
#define DDR_BUFFER_CNT            3               ///< number of DDR buffers per ISP stream
#define FRMS_CONTINUOUS_LOSE      3
#define FRMS_TO_CAPTURE           1//stark modify for 20190506

#define EXPHIST_ENGINE 0
#if (EXPHIST_ENGINE==0)
  #define SEQ_EXP_HIST SEQ_IPUS0
#elif (EXPHIST_ENGINE==1)
  #define SEQ_EXP_HIST SEQ_IPUS1
#endif

#define EXPOSURE_ENGINE 1,EXPHIST_ENGINE
#define CHGAIN_ENGINE   1,EXPHIST_ENGINE

#define HDR_ENGINE  1,EXPHIST_ENGINE
#define HDR_ALPHA   0x78

#define VIGNETTING_ENGINE 1,2
#define DENOISE_ENGINE    0,2
#define OUTLUT_ENGINE     1,3
#define OUTLUT_IPU        IPUS_3

#define MIPI_SIMPLE_GRAPH		0
#define MIPI_SIMPLE_1_GRAPH		1
//#define DUAL_EXP_GRAPH        1

#ifdef __STANDALONE__
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
  sdi_grabber *mpGrabber;       ///< pointer to grabber instance
  sdi_FdmaIO  *mpFdma;          ///< pointer to fdma object
  SDI_Frame    mFrameIsp;       ///< currently fetched SDI frame
  uint8_t      mExpCnt;         ///< counter of exposure updates
  uint32_t     mExpVal;         ///< latest exposure control result

  uint8_t      mGraphIdx;
  enum CSI_IDX mcamIdx;

  // exposure and channel gain control
  uint32_t      mExpRange;
  SEQ_IpuHist_t mIpuHist;
  uint16_t      mLAlphaVal;
  SONY_ChGain_t mChGainLevel;
  SONY_ChGain_t mChGainHist;

  // Graph data
  SEQ_Head_Ptr_t* mpGraph;
  GraphMetadata_t* mGraphMetadata;
  char* mkmem_srec;
  char* msequencer_srec;

  // ** event counters and flags **
  bool     mError;                  ///< to signal ISP problems
  bool     mFrmDone;                ///< to signal frame was finished by ISP
  uint32_t mFrmCnt;                 ///< number of frames grabbed by the app
  uint32_t mFrmDoneCnt;             ///< number frame done events
  uint32_t mFrmLostContinuousCnt;   ///< number continuous frame lose
  uint32_t mFrmLostCnt;             ///< number frame lose
  uint32_t mFrmDoneCntLast;

  unsigned long mTimeFrmStart;  ///< frame interval start
}; // struct AppContext

/************************************************************************/
/** Prepare everything before executing the main functionality .
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, <0 otherwise
  ************************************************************************/
int32_t Prepare(AppContext &arContext, uint8_t graphIdx, uint8_t csiPort);

/************************************************************************/
/** Execute main functionality of the application.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, <0 otherwise
  ************************************************************************/
#ifdef __STANDALONE__
int32_t Run(AppContext &arContext, io::FrameOutputDCU &lDcu);
#else // #ifdef __STANDALONE__
int32_t Run(AppContext &arContext, io::FrameOutputV234Fb &lDcu);
#endif // else from #ifdef __STANDALONE__

/************************************************************************/
/** Cleanup all resources before application end.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, <0 otherwise
  ************************************************************************/
int32_t Cleanup(AppContext &arContext);

/************************************************************************/
/** Stops. graph execution.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, <0 otherwise
  ************************************************************************/
int32_t Stop(AppContext &arContext);

/************************************************************************/
/** Prepares required libraries.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, != 0 otherwise
  ************************************************************************/
int32_t LibsPrepare(AppContext &arContext);

/************************************************************************/
/** Prepares DDR buffers.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, != 0 otherwise
  ************************************************************************/
static int32_t DdrBuffersPrepareForCam2(AppContext &arContext);

/************************************************************************/
/** Prepares DDR buffers.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, != 0 otherwise
  ************************************************************************/
static int32_t DdrBuffersPrepareForCam1(AppContext &arContext);

/************************************************************************/
/** configure output LUT.
  *
  * \param  aLut 0: linear 1: gamma 2: 12to8bit 3: 16to8bit
  *
  * \return 0 if all ok, <0 otherwise
  ************************************************************************/
static int32_t SetOutLut(uint8_t aLut);

/************************************************************************/
/** Configures camera parameters and preprocessing.
  *
  * \param  arContext application context structure
  *
  * \return 0 if all ok, <0 otherwise
  ************************************************************************/
static int32_t CamConfig(AppContext &arContext);

/************************************************************************/
/** Initial setup of application context.
  *
  * \param arContext structure capturing the context of the application
  ************************************************************************/
static int ContextInit(AppContext &arContext, uint8_t graphIdx, uint8_t csiPort);

#ifndef __STANDALONE__
/************************************************************************/
/** SIGINT handler.
  *
  * \param  aSigNo
  ************************************************************************/
static void SigintHandler(int);

/************************************************************************/
/** SIGINT handler.
  *
  * \param  aSigNo
  *
  * \return SEQ_LIB_SUCCESS if all ok
  *         SEQ_LIB_FAILURE if failed
  ************************************************************************/
static int32_t SigintSetup(void);
#endif // #ifndef __STANDALONE__

static bool sStop = false;
static int8_t sCsiPort = 2;//stark modify

//***************************************************************************
// external variables
//***************************************************************************


#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
using namespace cv;
#include <umat.hpp>
#include <apex.h>
#include <oal.h>
#include <stdio.h>
#include <common_stringify_macros.h>

#define RectWidth	640	//960/* 1920x1080 */  //640/* 1280x720 */
#define RectHeight	360	//540/* 1920x1080 */  //360/* 1280x720 */


int main(int argc, char ** argv)
{
  int lRet = 0;
  AppContext lContext_Graph1;
  AppContext lContext_Graph2;




						//*** Init DCU Output ***
#ifdef __STANDALONE__
						io::FrameOutputDCU lDcuOutput(
											WIDTH,
											HEIGHT,
											io::IO_DATA_DEPTH_08,
											CHNL_CNTA);
#else // #ifdef __STANDALONE__
						io::FrameOutputV234Fb lDcuOutput(
												WIDTH,
												HEIGHT,
												io::IO_DATA_DEPTH_08,
												CHNL_CNTA);
#endif // else from #ifdef __STANDALONE__


  

  //*** process command line parameters ***
  printf("\n**************************************************************\n"
         "** Camera and graph switching demo: Alternating two graphs execution.\n"
         "** Description:\n"
         "**  o Run Sony imx224 cameras (on MipiCsi_0 or MipiCsi_1 or both) expected as image input.\n"
         "**  o mipi_simple and sony_dualexp graph are alternated.\n"
         "**  o Resulting image is displayed live using DCU.\n"
         "**  o After %u frames captured , the graph is restarted and swap to next one.\n"
         "*********************************\n\n"
         "** Usage:\n"
         "**  o ./isp_sonyimx224_csi_dcu_cam_switch.elf [<csi port>]\n"
         "**\n"
         "** Options:\n"
         "**  o csi port         0|1|2    [default: 2]\n"
         "**    o csi port       0        Single camera runs on MipiCsi_0\n"
         "**    o csi port       1        Single camera runs on MipiCsi_1\n"
         "**    o csi port       2        Dual cameras runs on MipiCsi_0 and MipiCsi_1\n"
         "**\n"
         "*********************************\n\n"
         "** Example:\n"
         "**  o Run single camera, Sony imx224 pluged in CSI #0.\n"
         "**    ./isp_sonyimx224_csi_dcu_cam_switch.elf 0\n"
         "**  o Run two cameras.\n"
         "**    ./isp_sonyimx224_csi_dcu_cam_switch.elf 2\n"
         "**  o Press Ctrl+C to terminate the demo.\n"
         "**************************************************************\n\n"
        , FRMS_TO_CAPTURE);

  if(argc > 1)
  {
    sCsiPort = atoi(argv[1]);
  }

  if (sCsiPort > 2)
  {
    printf("Invalid input parameter, please read the help above\n");
    return lRet;
  }

#ifndef __STANDALONE__
    // setup Ctrl+C handler
  if(SigintSetup() != SEQ_LIB_SUCCESS)
  {
    VDB_LOG_ERROR("Failed to register Ctrl+C signal handler.");
    lRet = -1;
    goto END;
  }
#endif // ifndef __STANDALONE__

#if 1 
  if(Prepare(lContext_Graph1, MIPI_SIMPLE_GRAPH, sCsiPort))	//stark modify: sCsiPort -> (uint8_t)0
  {
      printf("1. graph failed in preparation phase at graph index 0.\n");
      lRet = -1;
      goto END;
  }
#endif

#if 1
  //if(Prepare(lContext_Graph2, DUAL_EXP_GRAPH, sCsiPort))
  if(Prepare(lContext_Graph2, MIPI_SIMPLE_1_GRAPH, sCsiPort))	//stark add
  {
      printf("2. graph failed in preparation phase at graph index 1.\n");
      lRet = -1;
      goto END;
  }
#endif


  while(!sStop)
  {

#if 1
    if(lContext_Graph1.mpGrabber->PreStart() == LIB_SUCCESS)
    {
      printf("Switch to Graph1\n");

	  //------------------------------------ Run ------------------------------
	  if(lContext_Graph1.mpGrabber->Start() != LIB_SUCCESS)
	  {
    	printf("Failed to start the grabber2.\n");
    	return -1;
  	  } // if Start() failed

	  for(;;)
	  {

	  
	  SDI_Frame lFrame1,lFrame,lFrameOutPut;  //stark add

	  SDI_Frame lFrame0;  //stark add
	  
    	lFrame1 = lContext_Graph1.mpGrabber->FramePop();
		lFrame0 = lContext_Graph1.mpGrabber->FramePop();
    	if(lFrame1.mUMat.empty())
		{
      		printf("Failed to grab image number %u\n", lContext_Graph1.mFrmCnt);
			return -1;
      	}


	cv::Mat lFrame_opencv=cv::mat (640,360,8UC3)
  // cv::Mat lFrame_opencv = lFrame1.mUMat.getMat(vsdk::ACCESS_READ | OAL_USAGE_CACHED);

   cv::Mat    output_mat = lFrame0.mUMat.getMat(ACCESS_WRITE | OAL_USAGE_CACHED);
   // cv::Mat lFrame_opencv = lFrame1.getmUMat(vsdk::ACCESS_READ | OAL_USAGE_CACHED);

		lDcuOutput.PutFrame(lFrame1.mUMat);

		if(lContext_Graph1.mpGrabber->FramePush(lFrame1) != LIB_SUCCESS)
    	{
      		printf("Failed to push image number %u\n", lContext_Graph1.mFrmCnt);
      		lContext_Graph1.mError = true;
      		return -1;
    	} // if push failed
	
      	break;
	  }
	  //------------------------------------ Run end ------------------------------

#if 0
      if(Run(lContext_Graph1,lDcuOutput) != 0)
      {
        printf("1. graph execution failed.\n");
        lRet = -1;
        goto END;
      } // if Run1() failed
#endif

      if(Stop(lContext_Graph1) != 0)
      {
        printf("1. graph stop failed.\n");
        lRet = -1;
        goto END;
      }
    }
    else
    {
      printf("1. graph failed in prestart phase.\n");
      lRet = -1;
      goto END;
    }
#endif


#if 1
if(lContext_Graph2.mpGrabber->PreStart() == LIB_SUCCESS)
    {
      printf("Switch to Graph2\n");

      if(Run(lContext_Graph2,lDcuOutput) != 0)
      {
        printf("2. graph execution failed.\n");
        lRet = -1;
        goto END;
      } // if Run1() failed

      if(Stop(lContext_Graph2) != 0)
      {
        printf("2. graph stop failed.\n");
        lRet = -1;
        goto END;
      }
    }
    else
    {
      printf("2. graph failed in prestart phase.\n");
      lRet = -1;
      goto END;
    }	
#endif
	
  } // while(!sStop)


END:
  // Clean up
  Cleanup(lContext_Graph1);
  Cleanup(lContext_Graph2);

  return lRet;
} // main()

//***************************************************************************

int32_t Prepare(AppContext &arContext, uint8_t graphIdx, uint8_t csiPort)
{

	printf("---------------[stark add]:Prepare(,,csiPort=%d)--------------------\n",csiPort);

  // init app context
  ContextInit(arContext, graphIdx, csiPort);

  // enable LIBS
  if(LibsPrepare(arContext) != 0)
  {
    printf("Failed to prepare libraries.\n");
    return -1;
  } // if failed to configure decoder
  // enable OAL


  if (arContext.mGraphIdx == 0)
  {
    if(DdrBuffersPrepareForCam1(arContext) != 0)
    {
      printf("Failed to prepare DDR buffers.\n");
      return -1;
    } // if fialed to prepare DDR buffers
  }
  else if (arContext.mGraphIdx == 1)
  {
    if(DdrBuffersPrepareForCam2(arContext) != 0)
    {
      printf("Failed to prepare DDR buffers.\n");
      return -1;
    } // if fialed to prepare DDR buffers
  }

  return 0;
} // Prepare()

//***************************************************************************

static int ContextInit(AppContext &arContext, uint8_t graphIdx, uint8_t csiPort)
{
  int lRet = 0;

  arContext.mpGrabber   = NULL;
  arContext.mpFdma      = NULL;
  arContext.mError      = false;
  arContext.mFrmCnt     = 0;
  arContext.mFrmDoneCnt = 0;
  arContext.mFrmLostCnt = 0;
  arContext.mFrmLostContinuousCnt = 0;
  arContext.mGraphIdx = graphIdx;

  if (csiPort == 0)
  {
    arContext.mcamIdx = CSI_IDX_0;
  }
  else if(csiPort == 1)
  {
    arContext.mcamIdx = CSI_IDX_1;
  }
  else if(csiPort == 2)
  {
    if (arContext.mGraphIdx == 0)
    {
      arContext.mcamIdx = CSI_IDX_0;
    }
    else if (arContext.mGraphIdx == 1)
    {
      arContext.mcamIdx = CSI_IDX_1;
    }
  }

  if (arContext.mGraphIdx == 0)
  {
    arContext.mpGraph = gpGraph_mipi_simple;
    arContext.mGraphMetadata = &gGraphMetadata_mipi_simple;
    arContext.mkmem_srec = kmem_mipi_simple_srec;
    arContext.msequencer_srec = sequencer_mipi_simple_srec;
  }
  else if (arContext.mGraphIdx == 1)
	{
	  arContext.mpGraph = gpGraph_mipi_simple_1;
	  arContext.mGraphMetadata = &gGraphMetadata_mipi_simple_1;
	  arContext.mkmem_srec = kmem_mipi_simple_1_srec;
	  arContext.msequencer_srec = sequencer_mipi_simple_1_srec;
	}



  #if 0
  else if (arContext.mGraphIdx == 1)
  {
    arContext.mpGraph = gpGraph_sony_dualexp;
    arContext.mGraphMetadata = &gGraphMetadata_sony_dualexp;
    arContext.mkmem_srec = kmem_sony_dualexp_srec;
    arContext.msequencer_srec = sequencer_sony_dualexp_srec;

#if defined(OUTLUT_16TO8)
    arContext.mLAlphaVal = 4;
#else
    arContext.mLAlphaVal = 128;
#endif
  }
#endif

  else
  {
    lRet = -1;
    printf("Not support graph index: %d\n", graphIdx);
  }


  printf("---------------[stark add]:Prepare().ContextInit(, graphIdx = %d , csiPort = %d )--------------------\n",graphIdx,csiPort);
  printf("---------------[stark add]:Prepare().ContextInit().arContext.mGraphIdx = %d --------------------\n",arContext.mGraphIdx);
  printf("---------------[stark add]:Prepare().ContextInit().arContext.mcamIdx = %d --------------------\n",(uint8_t)arContext.mcamIdx);


  return lRet;
} // ContextInit()

//***************************************************************************


int32_t LibsPrepare(AppContext &arContext)
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

  if(arContext.mpGrabber->ProcessSet(
    arContext.mpGraph,
    arContext.mGraphMetadata,
    arContext.mkmem_srec,
    arContext.msequencer_srec) != LIB_SUCCESS)
  {
    printf("Failed to set ISP graph to grabber.\n");
    return -1;
  } // if ISP graph not set

	//sCsiPort = (int8_t)arContext.mcamIdx;//stark add

#if 1
  if (( sCsiPort == 1) || (arContext.mcamIdx == CSI_IDX_1 && sCsiPort == 2))
  {
  	printf("!!!!!!!!!!!!!!!!!!!!!![stark add]: (( sCsiPort == 1) || (arContext.mcamIdx == CSI_IDX_1 && sCsiPort == 2)) !!!!!!!!!!!!!!!!!!!!!!\n");
    if(arContext.mpGrabber->IoGet(SEQ_OTHRIX_MIPICSI0))
    {

	  printf("!!!!!!!!!!!!!!!!!!!!!![stark add]: (arContext.mpGrabber->IoGet(SEQ_OTHRIX_MIPICSI0)) !!!!!!!!!!!!!!!!!!!!!!\n");
      arContext.mpGrabber->CsiSwap(1 + SEQ_OTHRIX_MIPICSI0, SEQ_OTHRIX_MIPICSI0);
    }
  }
#endif

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

static int32_t DdrBuffersPrepareForCam1(AppContext &arContext)
{

	printf("---------------[stark add]:Prepare().DdrBuffersPrepareForCam1() --------------------\n");

  // *** RGB full buffer array ***
  // modify DDR frame geometry to fit display output
  SDI_ImageDescriptor lFrmDesc;
  lFrmDesc = SDI_ImageDescriptor(WIDTH, HEIGHT, RGB888);

  if(arContext.mpFdma->DdrBufferDescSet(FDMA_IX_FastDMA_Out_MIPI_SIMPLE,
                                        lFrmDesc) != LIB_SUCCESS)
  {
    printf("Failed to set image descriptor.\n");
    return -1;
  } // if frame descriptor setup failed

  // allocate DDR buffers
  if(arContext.mpFdma->DdrBuffersAlloc(DDR_BUFFER_CNT) != LIB_SUCCESS)
  {
    printf("Failed to allocate DDR buffers.\n");
    return -1;
  } // if ddr buffers not allocated

  return 0;
} // DdrBuffersPrepareForCam1(AppContext &arContext)

//***************************************************************************


static int32_t DdrBuffersPrepareForCam2(AppContext &arContext)
{

	printf("---------------[stark add]:Prepare().DdrBuffersPrepareForCam2() --------------------\n");

  // *** RGB full buffer array ***
  // modify DDR frame geometry to fit display output
  SDI_ImageDescriptor lFrmDesc;
  lFrmDesc = SDI_ImageDescriptor(WIDTH, HEIGHT, RGB888);

  if(arContext.mpFdma->DdrBufferDescSet(FDMA_IX_FastDMA_Out_1_MIPI_SIMPLE_1,
                                        lFrmDesc) != LIB_SUCCESS)
  {
    printf("Failed to set image descriptor.\n");
    return -1;
  } // if frame descriptor setup failed

  // allocate DDR buffers
  if(arContext.mpFdma->DdrBuffersAlloc(DDR_BUFFER_CNT) != LIB_SUCCESS)
  {
    printf("Failed to allocate DDR buffers.\n");
    return -1;
  } // if ddr buffers not allocated

  return 0;
} // DdrBuffersPrepareForCam1(AppContext &arContext)

//***************************************************************************



#if 0
static int32_t DdrBuffersPrepareForCam2(AppContext &arContext)
{

	printf("---------------[stark add]:Prepare().DdrBuffersPrepareForCam2() --------------------\n");

  // *** UYVY full buffer array ***
  // modify DDR frame geometry to fit display output
  SDI_ImageDescriptor lFrmDesc;
  lFrmDesc = SDI_ImageDescriptor
  (
    WIDTH,
    NUM_OUT_LINES >> 0,
    YUV422Stream_UYVY
  );

  if(arContext.mpFdma->DdrBufferDescSet(FDMA_IX_ISP_UYVY_OUTPUT_SONY_DUALEXP,
                                        lFrmDesc) != LIB_SUCCESS)
  {
    printf("Failed to set image descriptor.\n");
    return -1;
  } // if frame descriptor setup failed

  // allocate DDR buffers
  if(arContext.mpFdma->DdrBuffersAlloc(DDR_BUFFER_CNT) != LIB_SUCCESS)
  {
    printf("Failed to allocate DDR buffers.\n");
    return -1;
  } // if ddr buffers not allocated

  return 0;
} // DdrBuffersPrepareForCam2(AppContext &arContext)
#endif

//***************************************************************************

#ifdef __STANDALONE__
int32_t Run(AppContext &arContext, io::FrameOutputDCU &lDcu)
#else // #ifdef __STANDALONE__
int32_t Run(AppContext &arContext, io::FrameOutputV234Fb &lDcu)
#endif // else from #ifdef __STANDALONE__
{
  unsigned long lTimeStart = 0, lTimeEnd = 0, lTimeDiff = 0;

  // *** start grabbing ***
  if(arContext.mpGrabber->Start() != LIB_SUCCESS)
  {
    printf("Failed to start the grabber.\n");
    return -1;
  } // if Start() failed

  GETTIME(&lTimeStart);
  //SDI_Frame lFrame;
  // *** grabbing/processing loop ***
  for(;;)
  {
    lFrame = arContext.mpGrabber->FramePop();
    if(lFrame.mUMat.empty())
    {
      printf("Failed to grab image number %u\n", arContext.mFrmCnt);
      arContext.mFrmLostCnt++;
      arContext.mFrmLostContinuousCnt++;
      if (arContext.mFrmLostContinuousCnt < FRMS_CONTINUOUS_LOSE)
      {
        printf("Restart the graph.\n");
        if(arContext.mpGrabber->Stop())
        {
          printf("Failed to stop the grabber.\n");
          return -1;
        }
        if(arContext.mpGrabber->Release())
        {
          printf("Failed to release grabber resources.\n");
          return -1;
        }
        if(arContext.mpGrabber->PreStart() != LIB_SUCCESS)
        {
          printf("Failed to pre start the graph.\n");
          return -1;
        }

#if 0//-------------------stark add
        if (arContext.mGraphIdx == DUAL_EXP_GRAPH)
        {
          if(CamConfig(arContext) != 0)
          {
            printf("Failed to set cam config.\n");
            return -1;
          } // if CamConfig() failed
        }
#endif


        if(arContext.mpGrabber->Start() != LIB_SUCCESS)
        {
          printf("Failed to start the grabber after restart graph.\n");
          return -1;
        } // if Start() failed
      }
      else
      {
        printf("Failed to recover the grabber at %d times. Please re-check and make sure all HW and config are right\n", arContext.mFrmLostContinuousCnt);
        return -1;
      }
      continue;
    } // if pop failed

    arContext.mFrmLostContinuousCnt = 0;
    arContext.mFrmCnt++;


    {
		lFrameOutPut.mUMat = vsdk::UMat(HEIGHT, WIDTH, VSDK_CV_8UC3);
	 	if (lFrameOutPut.mUMat.empty())
	 	{
		  printf("--------------[stark add]Failed to allocate DDR display buffer.");
		  return(-1);
		} 

		cv::Mat output_mat1 = lFrameOutPut.mUMat.getMat(vsdk::ACCESS_WRITE | OAL_USAGE_CACHED);
		cv::Mat input_mat1 = lFrame1.mUMat.getMat(vsdk::ACCESS_READ | OAL_USAGE_CACHED);
		cv::Mat input_mat2 = lFrame.mUMat.getMat(vsdk::ACCESS_READ | OAL_USAGE_CACHED);
	
		input_mat1(Rect(0,0,RectWidth,RectHeight)).copyTo(output_mat1((Rect(RectWidth,0,RectWidth,RectHeight))));
		input_mat2(Rect(0,0,RectWidth,RectHeight)).copyTo(output_mat1((Rect(RectWidth,RectHeight,RectWidth,RectHeight))));
	}




    lDcu.PutFrame(lFrameOutPut.mUMat);

    if(arContext.mpGrabber->FramePush(lFrame) != LIB_SUCCESS)
    {
      printf("Failed to push image number %u\n", arContext.mFrmCnt);
      arContext.mError = true;
      return -1;
    } // if push failed


    if((FRMS_TO_CAPTURE > 0) && (arContext.mFrmCnt == FRMS_TO_CAPTURE))
    {
      GETTIME(&lTimeEnd);
      lTimeDiff  = lTimeEnd - lTimeStart;
      lTimeStart = lTimeEnd;

      printf("%u frames took %lu usec (%5.2ffps)\n%u frames lose\n",
             FRMS_TO_CAPTURE,
             lTimeDiff,
            (FRMS_TO_CAPTURE*1000000.0)/((float)lTimeDiff),
            arContext.mFrmLostCnt
            );
      break;
    }
	
  }

  return 0;
} // Run()

//***************************************************************************

int32_t Stop(AppContext &arContext)
{
  int32_t lRet = 0;
  arContext.mError      = false;
  arContext.mFrmCnt     = 0;
  arContext.mFrmDoneCnt = 0;
  arContext.mFrmLostCnt = 0;

  if(arContext.mpGrabber != NULL)
  {
    if(arContext.mpGrabber->Stop())
    {
      printf("Failed to stop the grabber.\n");
      lRet = -1;
    }
    if(arContext.mpGrabber->Release())
    {
      printf("Failed to release grabber resources.\n");
      lRet = -1;
    }
  }

  return lRet;
} // Stop()

//***************************************************************************

int32_t Cleanup(AppContext &arContext)
{
  int32_t lRet = 0;

  if(arContext.mpGrabber != NULL)
  {
    Stop(arContext);

    delete(arContext.mpGrabber);
    arContext.mpGrabber = NULL;
  } // if grabber exists

  if(sdi::Close(0) != LIB_SUCCESS)
  {
    printf("Failed to terminate use of SDI.\n");
    lRet = -1;
  } // if SDI use termination failed

  return lRet;
} // Cleanup()

//***************************************************************************

static int32_t CamConfig(AppContext &arContext)
{
  // *** exposure kernel ***
  // black level correction
  if(seq_setReg(
      CHGAIN_ENGINE,
      0,
      0x70,
      232<<4) != SEQ_LIB_SUCCESS) // GPR0: R
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
      CHGAIN_ENGINE,
      0,
      0x71,
      232<<4) != SEQ_LIB_SUCCESS) // GPR1: GR
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
      CHGAIN_ENGINE,
      0,
      0x72,
      232<<4) != SEQ_LIB_SUCCESS) // GPR2: GB
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
      CHGAIN_ENGINE,
      0,
      0x73,
      232<<4) != SEQ_LIB_SUCCESS) // GPR3: B
  {
    return -1;
  } // if seq_setReg() failed

  // channel gain
  if(seq_setReg(
       CHGAIN_ENGINE,
       0,
       0x74,
       400) != SEQ_LIB_SUCCESS) // GPR4: R
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
       CHGAIN_ENGINE,
       0,
       0x75,
       256) != SEQ_LIB_SUCCESS) // GPR5: GR
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
       CHGAIN_ENGINE,
       0,
       0x76,
       256) != SEQ_LIB_SUCCESS) // GPR6: GB
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
       CHGAIN_ENGINE,
       0,
       0x77,
       450) != SEQ_LIB_SUCCESS) // GPR7: B
  {
    return -1;
  } // if seq_setReg() failed

  if(seq_setReg(
       HDR_ENGINE,
       0,
       HDR_ALPHA,
       16) != SEQ_LIB_SUCCESS)
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
       HDR_ENGINE,
       0,
       HDR_ALPHA + 1,
       240) != SEQ_LIB_SUCCESS)
  {
    return -1;
  } // if seq_setReg() failed

#ifdef SEPARATE_EXP_HDR
  // *** tonemap kernel ***
  if(seq_setReg(
       HDR_ENGINE,
       1,
       0x120,
       0x00350000 |
       (256 - arContext.mLAlphaVal) != SEQ_LIB_SUCCESS)//CH2_CFG_INA
  {
    return -1;
  } // if seq_setReg() failed
#endif

  // *** vignetting kernel ***
#define VIGNETTING_LUT_OFFSET 0
  if(seq_setReg(
       VIGNETTING_ENGINE,
       0,
       0x70,
       644) != SEQ_LIB_SUCCESS)   // GPR0 (IPUS)
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
       VIGNETTING_ENGINE,
       0,
       0x71,
       364) != SEQ_LIB_SUCCESS)   // GPR1 (IPUS)
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
       VIGNETTING_ENGINE,
       0,
       0x72,
       115) != SEQ_LIB_SUCCESS)    // GPR2 (IPUS)
  {
    return -1;
  } // if seq_setReg() failed

  if(seq_setReg(
       VIGNETTING_ENGINE,
       0,
       0x73,
       1024) != SEQ_LIB_SUCCESS)   // GPR0 (IPUS)
  {
    return -1;
  } // if seq_setReg() failed
  if(seq_setReg(
       VIGNETTING_ENGINE,
       1,
       0x78,
       ((VIGNETTING_LUT_OFFSET)>>1)) == SEQ_LIB_SUCCESS) // LUTA (IPUS)
  {
    uint16_t lutcnt=0;
    while (lutcnt < 1024)
    {
      uint32_t val;
      val=vignetting[lutcnt++];
      val|=((long)vignetting[lutcnt++]<<16);
      if(seq_setReg(
           VIGNETTING_ENGINE,
           1,
           0x7C,
           val) != SEQ_LIB_SUCCESS)  // LUTD (IPUS)
      {
        return -1;
      }// if seq_setReg() succeeded
    } // while lutcnt < 1024
  }// if seq_setReg() succeeded
  else
  {
    return -1;
  } // else if seq_setReg() failed

  // *** denoise kernel ***
  if(seq_setReg(
       DENOISE_ENGINE,
       0,
       0x56,
       0x300) != SEQ_LIB_SUCCESS) // GPR6 (IPUV): Noise level
  {
    return -1;
  } // if seq_setReg() failed

  // *** Output LUT
  if(SetOutLut(2) != 0)
  {
    return -1;
  }

  // *** Camera Configuration ***
  // modify camera geometry !!! setup before setting up expsoure control
  SONY_Geometry_t lGeo;
  lGeo.mCsiIdx  = arContext.mcamIdx;
  if(SONY_GeometryGet(lGeo) != CAM_LIB_SUCCESS)
  {
    printf("Failed to get camera geometry.\n");
    return -1;
  } // if SONY_GeometryGet failed

  lGeo.mFps     = 30;
  lGeo.mExpMode = SONY_DUAL_EXP;
  if(SONY_GeometrySet(lGeo) != CAM_LIB_SUCCESS)
  {
    printf("Failed to set camera geometry.\n");
    return -1;
  } // if SONY_GeometrySet failed
  // *** enable IPU histogram readout ***
  memset(&arContext.mIpuHist, 0, sizeof(SEQ_IpuHist_t));
  arContext.mIpuHist.mIpuIdx = SEQ_EXP_HIST;
  SEQ_HistogramEnable(SEQ_EXP_HIST);

  // *** setup initial exposure ***
  SONY_Exposure_t lExp;
  lExp.mCsiIdx = arContext.mcamIdx;

  if(SONY_ExposureGet(lExp) != CAM_LIB_SUCCESS)
  {
    printf("Failed to get camera exposure config.\n");
    return -1;
  } // if SONY_ExposureGet failed
  lExp.mAnalogGain     = 0;
  lExp.mConversionGain = 1;
  lExp.mExposureLines  = 500<<1;
  lExp.mConfigFlags    = SONY_EXP_EL | SONY_EXP_CG | SONY_EXP_AG;
  if(SONY_ExposureSet(lExp) != CAM_LIB_SUCCESS)
  {
    printf("Failed to set camera exposure config.\n");
    return -1;
  } // if SONY_ExposureSet failed

  if(SONY_GeometryGet(lGeo) != CAM_LIB_SUCCESS)
  {
    printf("Failed to get camera geometry.\n");
    return -1;
  } // if SONY_GeometryGet failed
  arContext.mExpRange = lGeo.mVmax;
  arContext.mExpRange<<=1;  // for dual exposure

  return 0;
} // CamConfig();

//***************************************************************************

static int32_t SetOutLut(uint8_t aLut)

{
  uint32_t val0;
  uint32_t val1;
  uint16_t lutcnt=0;
#if defined (__STANDALONE__)
  volatile struct IPUS_tag *lpOutLut=&(OUTLUT_IPU);
#endif
#define OUTLUT_OFFSET 0
#define OUTLUT_SIZE 4096

  // GPR1 (IPUS)
  if(seq_setReg(OUTLUT_ENGINE, 0, 0x71, OUTLUT_OFFSET) != SEQ_LIB_SUCCESS)
  {
    return -1;
  } // if seq_setReg() failed
  // LUTA
#if defined (__STANDALONE__)
  lpOutLut->HOST_LUTA.R=OUTLUT_OFFSET>>1;
#else
  if(seq_setReg(
       OUTLUT_ENGINE,
       1,
       0x78,
       ((OUTLUT_OFFSET)>>1)) != SEQ_LIB_SUCCESS)
  {
    return -1;
  } // if seq_setReg() failed
#endif

  if (aLut == 0)
  {
    while (lutcnt < OUTLUT_SIZE)
    {
      val0=512;//((lutcnt++)<<4);  // including 15 to 16 bit conversion
      lutcnt++;
      val1=512;//((lutcnt++)<<4);
      lutcnt++;
#if defined (__STANDALONE__)
      lpOutLut->HOST_LUTD.R=val0 | (val1<<16);
#else
      if(seq_setReg(
           OUTLUT_ENGINE,
           1,
           0x7C,
           val0 | (val1<<16)) != SEQ_LIB_SUCCESS)

      {
        return -1;
      } // if seq_setReg() failed
#endif
    } // while lutcnt < OUTLUT_SIZE
  } // if lut == 0
  else if (aLut == 1)
  {
#define GAMMA  0.8
#define GAMMA_SCALE  (0xFFF0)
    while (lutcnt < OUTLUT_SIZE)
    {
      if (!lutcnt)
      {
        val0=0;
      }
      else
      {
        val0=(uint32_t)
             (  256.0*   // 8.8 factors
                ((float)GAMMA_SCALE)*
                pow(((float)lutcnt)/((float)(OUTLUT_SIZE-1)),
                GAMMA) /
                (float(lutcnt<<3)) // 15 bit input values
             );
      }
      lutcnt++;
      val1=(uint32_t)
           (  256.0*   // 8.8 factors
              ((float)GAMMA_SCALE)*
              pow(((float)lutcnt)/((float)(OUTLUT_SIZE-1)),
              GAMMA) /
              (float(lutcnt<<3)) // 15 bit input values
           );
      lutcnt++;
      if (val0>0xffff)
      {
        val0=0xffff;
      }
      if (val1>0xffff)
      {
        val1=0xffff;
      }
#if defined (__STANDALONE__)
      lpOutLut->HOST_LUTD.R=val0 | (val1<<16);
#else
      if(seq_setReg(
           OUTLUT_ENGINE,
           1,
           0x7C,
           val0 | (val1<<16)) != SEQ_LIB_SUCCESS)
      {
        return -1;
      } // if seq_setReg() failed
#endif
    } // while lutcnt < OUTLUT_SIZE
  }
  else if (aLut == 2) {
#define LOG_A_12TO8 15453.32
#define LOG_B_12TO8 0.016704
    while (lutcnt < OUTLUT_SIZE)
    {
      if (!lutcnt)
      {
        val0=0;
      }
      else
      {
        val0=(uint32_t)
             (  256.0*
                LOG_A_12TO8*logf((lutcnt)*LOG_B_12TO8+1) /
                ((float)(lutcnt<<3))
             );
      }
      lutcnt++;
      val1=(uint32_t)
           (  256.0*
              LOG_A_12TO8*logf((lutcnt)*LOG_B_12TO8+1) /
              ((float)(lutcnt<<3))
           );
      lutcnt++;
      if(val0>0xffff)
      {
        val0 = 0xffff;
      }
      if(val1>0xffff)
      {
        val1 = 0xffff;
      }
#if defined (__STANDALONE__)
      lpOutLut->HOST_LUTD.R=val0 | (val1<<16);
#else
      if(seq_setReg(
           OUTLUT_ENGINE,
           1,
           0x7C, val0 | (val1<<16)) != SEQ_LIB_SUCCESS)
      {
        return -1;
      } // if seq_setReg() failed
#endif
    } // while lutcnt < OUTLUT_SIZE
  }
  else if (aLut == 3)
  {
#define LOG_A_16TO8 8332.00
#define LOG_B_16TO8 0.634928
    uint16_t lutcnt=0;
    while (lutcnt < OUTLUT_SIZE)
    {
      if (!lutcnt)
      {
        val0=0x0;
      }
      else
      {
        val0=(uint32_t)
             (  256.0*
                LOG_A_16TO8*logf((lutcnt)*LOG_B_16TO8+1) /
                ((float)(lutcnt<<3))
             );
      }
      lutcnt++;
      val1=(uint32_t)
           (  256.0*
              LOG_A_16TO8*logf((lutcnt)*LOG_B_16TO8+1) /
              ((float)(lutcnt<<3))
           );
      lutcnt++;

      if(val0>0xffff)
      {
        val0=0xffff;
      }
      if(val1>0xffff)
      {
        val1=0xffff;
      }
#if defined (__STANDALONE__)
      lpOutLut->HOST_LUTD.R=val0 | (val1<<16);
#else
      if(seq_setReg(
           OUTLUT_ENGINE,
           1,
           0x7C,
           val0 | (val1<<16)) != SEQ_LIB_SUCCESS)
      {
        return -1;
      } // if seq_setReg() failed
#endif
    } // while lutcnt < OUTLUT_SIZE
  }
  return 0;
} // SetOutLut()

//***************************************************************************

#ifndef __STANDALONE__
static void SigintHandler(int)
{
  sStop = true;
} // SigintHandler()

//***************************************************************************

static void SeqEventCallBack(uint32_t aEventType, void* apUserVal)
{
  AppContext *lpAppContext = (AppContext*)apUserVal;

  if(lpAppContext)
  {
    if(aEventType == SEQ_MSG_TYPE_FRAMEDONE)
    {
      printf("Frame done message arrived #%u.\n",
            lpAppContext->mFrmDoneCnt++);
    } // if frame done arrived
  } // if user pointer is NULL
} // SeqEventCallBack()

//***************************************************************************

static int32_t SigintSetup(void)
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
#endif // #ifndef __STANDALONE__

/* EOF */
