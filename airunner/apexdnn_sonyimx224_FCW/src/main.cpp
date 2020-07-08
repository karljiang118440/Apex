/*****************************************************************************
*
* Freescale Confidential Proprietary
*
* Copyright (c) 2016 Freescale Semiconductor, Inc.
* Copyright (c) 2016-2017 NXP
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
#define AirParam true

#if AirParam

#include <airunner_postprocessing_ssd.hpp>

#include "test_cases.hpp"
#include "apex.h"

using Clock = std::chrono::high_resolution_clock;
using namespace airunner;

template<typename T> std::vector<std::pair<int ,float>> processResults(T* aResults,int aNumResults){
  std::vector<std::pair<int,float>> resultPairs;
}


inline static void stopwatch(bool start, std::string verb = "")
{
  static auto startTime = Clock::now();

  if(start)
  {
    startTime = Clock::now();
  }
  else
  {
    auto endTime = Clock::now();
    std::cout << "Time taken to " << verb << ": "
              << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()
              << " milliseconds" << std::endl;
  }
}

#endif




#define  video_airunner 1 // 定义 FCW 对视频进行检测





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
#include "mipi_simple_c.h"

#include "vdb_log.h"
#include <common_helpers.h>

//***************************************************************************
// constants
//***************************************************************************

// Possible to set input resolution (must be supported by the DCU)
#define WIDTH1 1280       ///< width of DDR buffer in pixels
#define HEIGHT1 720       ///< height of DDR buffer in pixels
#define DDR_BUFFER_CNT 3 ///< number of DDR buffers per ISP stream

//***************************************************************************

#define FRM_TIME_MSR 300 ///< number of frames to measure the time and fps

//***************************************************************************
// macros
//***************************************************************************

#ifdef __STANDALONE__
//extern SEQ_Buf_t  producerless_buffer_1;
extern "C" {
unsigned long get_uptime_microS(void);
}

#define GETTIME(time) (*(time) = get_uptime_microS())
#else // ifdef __STANDALONE__
#define GETTIME(time)                                                                                                  \
  {                                                                                                                    \
    struct timeval lTime;                                                                                              \
    gettimeofday(&lTime, 0);                                                                                           \
    *time = (lTime.tv_sec * 1000000 + lTime.tv_usec);                                                                  \
  }
#endif // else from #ifdef __STANDALONE__

//***************************************************************************
// types
//***************************************************************************
struct AppContext
{
  sdi_grabber* mpGrabber; ///< pointer to grabber instance
  sdi_FdmaIO*  mpFdma;    ///< pointer to fdma object

  // ** event counters and flags **
  bool     mError;      ///< to signal ISP problems
  uint32_t mFrmDoneCnt; ///< number of frames done events
  uint32_t mFrmCnt;     ///< number of frames grabbed by the app
};                      // struct AppContext

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
static int32_t Prepare(AppContext& arContext);

/************************************************************************/
/** Initial setup of application context.
  *
  * \param arContext structure capturing the context of the application
  ************************************************************************/
static void ContextInit(AppContext& arContext);

/************************************************************************/
/** Prepares required libraries.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, != 0 otherwise
  ************************************************************************/
static int32_t LibsPrepare(AppContext& arContext);

/************************************************************************/
/** Prepares DDR buffers.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, != 0 otherwise
  ************************************************************************/
static int32_t DdrBuffersPrepare(AppContext& arContext);

/************************************************************************/
/** Execute main functionality of the application.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, <0 otherwise
  ************************************************************************/
static int32_t Run(AppContext& arContext);

/************************************************************************/
/** Cleanup all resources before application end.
  *
  * \param arContext structure capturing the context of the application
  *
  * \return 0 if all ok, <0 otherwise
  ************************************************************************/
static int32_t Cleanup(AppContext& arContext);

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

//*****************************************************************************
// static variables
//*****************************************************************************

static bool sStop = false; ///< to signal Ctrl+c from command line

#endif // #ifndef __STANDALONE__










int main(int argc, char** argv)
{

  #if AirParam

  APEX_Init();

  #endif 

  int lRet = 0;

  AppContext lContext;

  //*** process command line parameters ***
  const char helpMsg_str[] =
      "\n**************************************************************\n"
      "** Sony imx224 csi -> dcu demo\n"
      "** Description:\n"
      "**  o Sony imx224 camera (on MipiCsi_0) expected as image input.\n"
      "**  o ISP does simple debayering.\n"
      "**  o Resulting RBG 1280x720 image is displayed live using DCU.\n"
      "**\n"
      "** Usage:\n"
      "**  o no cmd line parameters available.\n"
      "**\n"
      "************************************************************\n\n";
  int idxHelp = COMMON_HelpMessage(argc, argv, helpMsg_str);
  if(idxHelp < 0)
  { // print help message even if no help option is provided by the user
    printf("%s", helpMsg_str);
  }

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
  }   // if Prepare() ok
  else
  {
    printf("Demo failed in preparation phase.\n");
    lRet = -1;
  } // else from if Prepare() ok

  if(Cleanup(lContext) != 0)
  {
    printf("Demo failed in cleanup phase.\n");
    lRet = -1;
  } // if cleanup failed

  return lRet;
} // main()

//***************************************************************************

static int32_t Prepare(AppContext& arContext)
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

#ifndef __STANDALONE__

#endif // #ifndef __STANDALONE__

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

  if(arContext.mpGrabber->SeqEventCallBackInstall(&SeqEventCallBack, &arContext) != LIB_SUCCESS)
  {
    printf("Failed to install Sequencer event callback.\n");
    return -1;
  } // if callback setup failed

  return 0;
} // Prepare()

//***************************************************************************

static void ContextInit(AppContext& arContext)
{
  arContext.mpGrabber   = NULL;
  arContext.mpFdma      = NULL;
  arContext.mError      = false;
  arContext.mFrmCnt     = 0;
  arContext.mFrmDoneCnt = 0;
} // ContextInit()

//***************************************************************************

static int32_t LibsPrepare(AppContext& arContext)
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
          gpGraph_mipi_simple, &gGraphMetadata_mipi_simple,
          kmem_mipi_simple_srec, sequencer_mipi_simple_srec) != LIB_SUCCESS)
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

static int32_t DdrBuffersPrepare(AppContext& arContext)
{
  // *** RGB full buffer array ***
  // modify DDR frame geometry to fit display output
  SDI_ImageDescriptor lFrmDesc;
  lFrmDesc = SDI_ImageDescriptor(WIDTH1, HEIGHT1, RGB888);

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
} // DdrBuffersPrepare(AppContext &arContext)

//***************************************************************************

static int32_t Run(AppContext& arContext)
{
//*** Init DCU Output ***
#ifdef __STANDALONE__
  io::FrameOutputDCU lDcuOutput(WIDTH, HEIGHT, io::IO_DATA_DEPTH_08, CHNL_CNT);
#else  // #ifdef __STANDALONE__
  // setup Ctrl+C handler
  if(SigintSetup() != SEQ_LIB_SUCCESS)
  {
    VDB_LOG_ERROR("Failed to register Ctrl+C signal handler.");
    return -1;
  }

  printf("Press Ctrl+C to terminate the demo.\n");

  io::FrameOutputV234Fb lDcuOutput(WIDTH1, HEIGHT1, io::IO_DATA_DEPTH_08, CHNL_CNT);
#endif // else from #ifdef __STANDALONE__

  unsigned long lTimeStart = 0, lTimeEnd = 0, lTimeDiff = 0;

  // *** start grabbing ***
  GETTIME(&lTimeStart);
  if(arContext.mpGrabber->Start() != LIB_SUCCESS)
  {
    printf("Failed to start the grabber.\n");
    return -1;
  } // if Start() failed





// 使用标准模型



#if 1

const std::string& aMssdGraph = "data/airunner/model/coco/ssdlite_mb2_stand_part_bn_quant_final.pb";


const std::string& aImageFile = "data/airunner/test_object_detection.jpg";
const std::string& aSlimLabelsFile = "data/airunner/object_detection/mscoco_labels.txt";
const std::string& target = "APEX";

const std::string& aLabels = "data/airunner/object_detection/mscoco_labels.txt";




cv::Mat   camera_mat;

int numClasses=91;
int anchorGenVer=2;


#endif 


#if 0 // 使用 kiiti 数据集

const std::string& aMssdGraph = "data/airunner/model/kitti/ssdlite_mb2_kitti_part_bn_quant_final.pb";
const std::string& aImageFile = "data/airunner/test_object_detection.jpg";
const std::string& aSlimLabelsFile = "data/airunner/object_detection/kitti_label_map.txt";
const std::string& target = "APEX";
const std::string& aLabels = "data/airunner/object_detection/kitti_label_map.txt";

cv::Mat   camera_mat;

int numClasses=3;
int anchorGenVer=2;





#endif 






#ifndef _WINDOWS

  std::cout << "Running MSSD graph for " << target << std::endl;

  Status_t status;

  // Load MSSD graph
  TargetInfo     lRefInfo  = TargetInfo();
  ApexTargetInfo lApexInfo = ApexTargetInfo();
  auto           lWorkspace =
      std::unique_ptr<Workspace>(new Workspace(std::map<std::string, TargetInfo*>{
          {target::REF, &lRefInfo}, {target::APEX, &lApexInfo}}));

  auto net_mssd = std::unique_ptr<Graph>(new Graph(lWorkspace.get()));

  Tensor* lApexNetInput = net_mssd->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
                       TensorShape<TensorFormat_t::NHWC>{1, 300, 300, 3}, 
                       TensorLayout<TensorFormat_t::NHWC>())));

  auto input = std::unique_ptr<Tensor>(new Tensor("TMP_INPUT"));

  lApexNetInput->SetQuantParams({QuantInfo(-1, 0)});
  lApexNetInput->Allocate(Allocation_t::OAL);

  // Integer output of the aMssdGraph
  std::vector<Tensor*> output;
  status = LoadNetFromTensorFlow(*net_mssd, aMssdGraph, lApexNetInput, output);
  if(Status_t::SUCCESS != status || output.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }
  if(target == "APEX")
  {
    status = net_mssd->SetTargetHint(target::APEX);

    if(Status_t::SUCCESS != status)
    {
      std::cout << "Set target failed" << std::endl;
      return {};
    }
  }
  else
  {
    status = net_mssd->SetTargetHint(target::REF);

    if(Status_t::SUCCESS != status)
    {
      std::cout << "Set target failed" << std::endl;
      return {};
    }
  }
  status = net_mssd->Prepare();

  if(Status_t::SUCCESS != status)
  {
    std::cout << " Net verification failed" << std::endl;
    return {};
  }

  int   numLayers             = 6;
  float minScale              = 0.2;
  float maxScale              = 0.95;
  float aspectRatios[]        = {1.f, 2.f, 1.f / 2, 3.0, 1.f / 3};
  int   numAspectRatios       = 5;
  int   numFeatureMaps        = 6;
  int   featureMapShapeList[] = {
      19, 19, 10, 10, 5, 5, 3, 3, 2, 2, 1, 1,
  };

  ssd::MultipleGridAnchorGenerator *anchorGenerator;
  ssd::createSsdAnchors(&anchorGenerator, numLayers, minScale, maxScale, numAspectRatios, aspectRatios,
                        nullptr, true, 1.0f, anchorGenVer);

  // Create default anchor lists anchorList
  BoxList* anchorList;
  ssd::generateAnchors(anchorGenerator, &anchorList, numFeatureMaps, featureMapShapeList);
  cornerToCentreBox(anchorList);

  // Decode boxes
  anchorParams *anchParams = new anchorParams;
  anchParams->y_scale = 10.0f;
  anchParams->x_scale = 10.0f;
  anchParams->height_scale = 5.0f;
  anchParams->width_scale  = 5.0f;

  std::ifstream            labelfile(aLabels);
  std::vector<std::string> classLabels;
  std::string              line;
  while(std::getline(labelfile, line))
  {
    classLabels.push_back(line);
  }

  std::vector<std::pair<int, float>> results;

  std::string imagePath = aImageFile;
  cv::Mat* outputImage = new cv::Mat();

  std::cout << "Detecting objects for: " << imagePath << std::endl;

#endif 





  SDI_Frame lFrame;
  // *** grabbing/processing loop ***



#ifdef video_airunner 
   cv::VideoCapture capture; /* open video */
	 capture.open("./data/object_detection/university_traffic.avi");
   
	 if(!capture.isOpened()) // if not success, exit program
	 {
	   printf("Cannot open the video file: \n");
	  //return -1;
	 } // if video open failed
   
   //cv::Mat outputImage = cv::Mat::zeros(cv::Size(1280, 720), CV_8UC3);



#endif 


  for(;;)
  {
    lFrame = arContext.mpGrabber->FramePop();
    if(lFrame.mUMat.empty())
    {
      printf("Failed to grab image number %u\n", arContext.mFrmCnt);
      arContext.mError = true;
      break;
    } // if pop failed



    arContext.mFrmCnt++;


 #if  AirParam 


cv::Mat camera_mat = lFrame.mUMat.getMat(vsdk::ACCESS_RW | OAL_USAGE_CACHED);

cv::flip(camera_mat,camera_mat,0); // 画面翻转



#ifndef _WINDOWS



  #ifndef video_airunner

    printf("mark2 \n");

 

     resizeBilinearAndNormalize(camera_mat, lApexNetInput, true, {128}, 1.0f);


  #else 



    cv::Mat outputImage;
    capture >>  outputImage;

   // printf("rows = %d ,cols = %d \n" , (outputImage.rows,outputImage.cols));
   // cv::imwrite("outputImage.jpg",outputImage);

  //printf("mark667 \n");

        resizeBilinearAndNormalize(outputImage, lApexNetInput, true, {128}, 1.0f);

  #endif 





  lApexNetInput->Flush();
  output[0]->Invalidate();
  output[1]->Invalidate();



  // 2. Run preprocessed img through MSSD to produce raw class and box prediction results
  stopwatch(true);
  status = net_mssd->Run();
  stopwatch(false, "mssd object detection");
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Net verification failed" << std::endl;
    return {};
  }


  Tensor* boxOutputFixedPoint   = nullptr;
  Tensor* classOutputFixedPoint = nullptr;

  for(auto& o : output)
  {
    if(o->Identifier() == "concat")
    {
      boxOutputFixedPoint = o;
    }
    if(o->Identifier() == "concat_1")
    {
      classOutputFixedPoint = o;
    }
  }
  if(!boxOutputFixedPoint || !classOutputFixedPoint)
  {
    std::cout << "Net output box/class not found " << std::endl;
    return {};
  }

  std::cout << boxOutputFixedPoint->Dim(TensorDim_t::BATCH) << " "
            << boxOutputFixedPoint->Dim(TensorDim_t::HEIGHT) << " "
            << boxOutputFixedPoint->Dim(TensorDim_t::WIDTH) << " "
            << boxOutputFixedPoint->Dim(TensorDim_t::OUTPUT_CHANNELS) << " " << std::endl; 

  std::cout << classOutputFixedPoint->Dim(TensorDim_t::BATCH) << " "
            << classOutputFixedPoint->Dim(TensorDim_t::HEIGHT) << " "
            << classOutputFixedPoint->Dim(TensorDim_t::WIDTH) << " "
            << classOutputFixedPoint->Dim(TensorDim_t::OUTPUT_CHANNELS) << " " << std::endl; 
 
  int numBoxOutput = boxOutputFixedPoint->Dim(TensorDim_t::BATCH)*
                     boxOutputFixedPoint->Dim(TensorDim_t::HEIGHT)*
                     boxOutputFixedPoint->Dim(TensorDim_t::WIDTH)*
                     boxOutputFixedPoint->Dim(TensorDim_t::OUTPUT_CHANNELS);
  int numClassOutput = classOutputFixedPoint->Dim(TensorDim_t::BATCH)*
                       classOutputFixedPoint->Dim(TensorDim_t::HEIGHT)*
                       classOutputFixedPoint->Dim(TensorDim_t::WIDTH)*
                       classOutputFixedPoint->Dim(TensorDim_t::OUTPUT_CHANNELS);

  if (numBoxOutput != 1917*4 && numClassOutput != 1917*numClasses)
  {
      std::cout << "Net output does not match SSD setting:" << numBoxOutput << " " << numClassOutput << std::endl;
      return {};
  }


  // 3. Apply post-processing to obtain final bounding box results
  BoxList* boxPredictorList  = new BoxList;
  boxPredictorList->numBoxes = 1917;
  boxPredictorList->boxList  = nullptr;

  tensorToFloatList(boxOutputFixedPoint, &boxPredictorList->boxList,
                    boxPredictorList->numBoxes * NUM_FLOAT_PER_BOX);
  ScoresList* scorePredictorList = new ScoresList;
  scorePredictorList->numBoxes   = 1917;
  scorePredictorList->numClasses = numClasses;
  scorePredictorList->scores     = nullptr;
  tensorToFloatList(classOutputFixedPoint, &scorePredictorList->scores,
                    scorePredictorList->numBoxes * scorePredictorList->numClasses);
  BoxList* boxes = boxDecoder(anchorList, boxPredictorList, anchParams);

  std::vector<BoundingBox> bboxes = multiclassNonMaxSuppression(boxes, scorePredictorList, 1, 0.35,
                                                                0.6, 100, 100, NULL, false, true);



  #ifndef video_airunner


  //cv::Mat outImage = *outputImage;

    cv::Mat outImage = camera_mat;

  #else 

    cv::Mat outImage = outputImage;

  #endif






  std::string imageResult = imagePath + ", " + std::to_string(bboxes.size()) + "\n";

  int index = 0;
  for(auto& a : bboxes)
  {
    float       ymin = outImage.rows * a.coord[0];
    float       xmin = outImage.cols * a.coord[1];
    float       ymax = outImage.rows * a.coord[2];
    float       xmax = outImage.cols * a.coord[3];
    std::string label =
        std::to_string(index) + " " + std::to_string(a.classNo) + " " + std::to_string(a.score);
    index++;
    int outlineWidth = 5;

    cv::Rect rec(xmin, ymin, xmax - xmin, ymax - ymin);
    cv::rectangle(outImage, rec, cv::Scalar(0, 250, 50), outlineWidth);

    int       fontface  = cv::FONT_HERSHEY_SIMPLEX;
    double    scale     = 0.3;
    int       thickness = 1;
    int       baseline  = 0;
    cv::Point origin    = cv::Point(xmin - outlineWidth, ymin - outlineWidth);
    cv::Size  text      = cv::getTextSize(label, fontface, scale, thickness, &baseline);
    cv::rectangle(outImage, origin + cv::Point(0, baseline),
                  origin + cv::Point(text.width, -text.height), CV_RGB(0, 0, 0), CV_FILLED);
    cv::putText(outImage, label, origin, cv::FONT_HERSHEY_SIMPLEX, scale, CV_RGB(255, 255, 255));
    imageResult += std::to_string(ymin) + ", " + std::to_string(xmin) + ", " +
                   std::to_string(ymax) + ", " + std::to_string(xmax) + ", " +
                   std::to_string(a.classNo) + ", " + std::to_string(a.score) + "\n";

    results.push_back(std::pair<int, float>(a.classNo, a.score));
  }

#if video_airunner


//cv::imwrite("outputImage.jpg",outImage);

 // vsdk::Mat lframe1 = outImage.getMat(OAL_USAGE_CACHED | ACCESS_READ);
 // vsdk::UMat lframe_umat = vsdk::UMat(720, 1280, VSDK_CV_8UC3);

 cv::resize(outImage,outImage,cv::Size(1280,720));

 vsdk::UMat lframe_umat = outImage.getUMat(cv::ACCESS_RW);

/*
   io::FrameOutputV234Fb output(1280, 720, io::IO_DATA_DEPTH_08, CHNL_CNT);

  
    // Output buffer (screen size) and it's mapped version (using cv mat in order to have copyTo functions)
    vsdk::UMat output_umat = vsdk::UMat(720, 1280, VSDK_CV_8UC3);
    {
      cv::Mat output_mat = output_umat.getMat(vsdk::ACCESS_WRITE | OAL_USAGE_CACHED);
      memset(output_mat.data, 0, 720 * 1280 * 3);
      outImage.copyTo(output_mat);
      //cv::imwrite("outputImage_1.jpg",output_mat);
    }

    output.PutFrame(output_umat);

    */




#endif 


  io_FrameOutput_t lFrameOutput;

  io::FrameOutputV234Fb output(1280, 720, io::IO_DATA_DEPTH_08, CHNL_CNT);

  //lFrameOutput.Init(1280, 1080, io::IO_DATA_DEPTH_08,io::IO_DATA_CH3);
  lFrameOutput.Init(1280, 720, io::IO_DATA_DEPTH_08,io::IO_DATA_CH3);


  std::cout << imageResult << std::endl;


/*

  outImage.release();
  deleteBoxList(boxPredictorList);
  deleteScoresList(scorePredictorList);
  results.clear();

  delete anchParams;
  deleteBoxList(anchorList);

*/

#else
  return {};
#endif


//} // end case_mssd_target


#endif



/*
#if 0

//添加计数功能，100帧 抓取一次画面

if(arContext.mFrmCnt%1 == 0){


  #if  AirParam 


cv::Mat camera_mat = lFrame.mUMat.getMat(vsdk::ACCESS_RW | OAL_USAGE_CACHED);



#ifndef _WINDOWS

const std::string& aMssdGraph = "data/airunner/model/coco/ssdlite_mb2_stand_part_bn_quant_final.pb";
const std::string& aImageFile = "data/airunner/test_object_detection.jpg";
const std::string& aSlimLabelsFile = "data/airunner/object_detection/mscoco_labels.txt";



  // Run APEX
  std::vector<Tensor*> apexOutput =
      case_mssd_target(aMssdGraph, aImageFile, camera_mat,aSlimLabelsFile, "APEX", 91, 2);
  if(apexOutput.empty())
  {
    std::cout << "Error: Empty APEX output tensor." << std::endl;
    return -1;
  }
#endif


#endif 



}


else
{
  lDcuOutput.PutFrame(lFrame.mUMat);

}


#endif 
*/


#ifndef video_airunner


        lDcuOutput.PutFrame(lFrame.mUMat);
#else


   // lDcuOutput.PutFrame(lframe_umat);
    //output.PutFrame(lframe_umat);

    //cv::imwrite("outImage_944.jpg",(cv::UMat)lframe_umat);

   lDcuOutput.PutFrame(outImage.data, false);


#endif 




    if(arContext.mpGrabber->FramePush(lFrame) != LIB_SUCCESS)
    {
      printf("Failed to push image number %u\n", arContext.mFrmCnt);
      arContext.mError = true;
      break;
    } // if push failed

    if((arContext.mFrmCnt % FRM_TIME_MSR) == 0)
    {
      GETTIME(&lTimeEnd);
      lTimeDiff  = lTimeEnd - lTimeStart;
      lTimeStart = lTimeEnd;

      printf("%u frames took %lu usec (%5.2ffps)\n", FRM_TIME_MSR, lTimeDiff,
             (FRM_TIME_MSR * 1000000.0) / ((float)lTimeDiff));
    } // if time should be measured
#ifndef __STANDALONE__
    if(sStop)
    {
      break; // break if Ctrl+C pressed
    }        // if Ctrl+C
#endif       //#ifndef __STANDALONE__
  }          // for ever

  return 0;
} // Run()

//***************************************************************************

static int32_t Cleanup(AppContext& arContext)
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
  for(;;)
    ;  // *** don't return ***
#endif // #ifdef __STANDALONE__

  if(sdi::Close(0) != LIB_SUCCESS)
  {
    printf("Failed to terminate use of SDI.\n");
    lRet = -1;
  } // if SDI use termination failed

  return lRet;
} // Cleanup()

//***************************************************************************

static void SeqEventCallBack(uint32_t aEventType, void* apUserVal)
{
  AppContext* lpAppContext = (AppContext*)apUserVal;

  if(lpAppContext)
  {
    if(aEventType == SEQ_MSG_TYPE_FRAMEDONE)
    {
     // printf("Frame done message arrived #%u.\n", lpAppContext->mFrmDoneCnt++);
    } // if frame done arrived
  }   // if user pointer is NULL
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

  if(sigaction(SIGINT, &lSa, NULL) != 0)
  {
    VDB_LOG_ERROR("Failed to register signal handler.\n");
    lRet = SEQ_LIB_FAILURE;
  } // if signal not registered

  return lRet;
} // SigintSetup()

//***************************************************************************
#endif // #ifndef __STANDALONE__
