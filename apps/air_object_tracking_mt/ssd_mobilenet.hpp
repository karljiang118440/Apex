/*****************************************************************************
*
* NXP Confidential Proprietary
*
* Copyright (c) 2019 NXP Semiconductor;
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

#ifndef __SSD_MOBILENET_HPP__
#define __SSD_MOBILENET_HPP__

#include "nxp_logo.h"
#include <preprocessing.hpp>
#include <postprocessing_ssd.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

#include "multi_threading.hpp"
#include "graph_runner.hpp"
#include "load_config.hpp"
#include "display_image.hpp"
#include "read_image.hpp"
#include "helper_utils.hpp"
#include "sumat.hpp"
#include "tracker.hpp"

#ifndef _WINDOWS
#include "sdi.hpp"
#include "mipi_simple_c.h"
#include "ov10635_surround_c.h"
#include "isp_cam_sony.h"
#endif

enum CameraType {
  ov10635,
  sonyimx224
};

// Run entire network
int SSDMobileNet(const std::unordered_map<std::string, std::string>& config);
int SSDMobileNetThread(const std::unordered_map<std::string, std::string>& config);
int SSDMobileNetDual(const std::unordered_map<std::string, std::string>& config);
int SSDMobileNetComp(const std::unordered_map<std::string, std::string>& config);

namespace airunner
{
//Group
struct SSDDatagram
{
  // unique id
  size_t uid = -1;
  size_t prof[5] = {0};
  // data
  Tensor* inTensor   = nullptr;
  Tensor* outTensor0 = nullptr;
  Tensor* outTensor1 = nullptr;
  cv::Mat image;
  bool valid = true;

  SSDDatagram()
  {
  }
  SSDDatagram(size_t id, Tensor* inT, Tensor* outT0, Tensor* outT1, cv::Mat img)
  {
    uid        = id;
    inTensor   = inT;
    outTensor0 = outT0;
    outTensor1 = outT1;
    image      = img;
    valid      = true;
  }
};

/*!****************************************************************************
* \ingroup ThreadingAPI
* \brief Implements image reader from file
******************************************************************************/
class ImageReader : public MultiThread<SSDDatagram, SSDDatagram>
{
public:
  ImageReader(std::string aPoolName, size_t aThreadCount) : MultiThread(aPoolName, aThreadCount)
  {
  }
  ~ImageReader()
  {
    if(mUseVideo)
    {
      mVideoCap.release();
    }
    else if (mUseCamera)
    {
#ifndef _WINDOWS
      //*** Stop ISP processing ***
      mSdiGrabber->Stop();
      // clean up grabber resources
      mSdiGrabber->Release();
      sdi::Close(0);
#endif
    }
  }

private:
  CameraType cameraType;
  std::vector<std::string> mInputImgPaths;
  int mIdx = 0;
  cv::VideoCapture             mVideoCap;
  bool                         mUseVideo = false;
  bool                         mUseCamera = false;
  bool                         mLoop = false;
  std::mutex mMutex;
#ifndef _WINDOWS
  std::unique_ptr<sdi_grabber> mSdiGrabber;
  vsdk::Rect mRect;
#endif
  uint32_t mImageSensorWidthInPixels = 1280;
  uint32_t mImageSensorHeightInPixels = 720;
  uint32_t mImageWidthInPixels = 1280;
  uint32_t mImageHeightInPixels = 720;

  bool loadImagePaths(std::string aImageNamesFilePath)
  {
    if(aImageNamesFilePath == "")
    {
      return false;
    }
    std::ifstream infile(aImageNamesFilePath);
    if(!infile.good())
    {
      std::cout << "Failed to read input image path(s) from: " << aImageNamesFilePath << std::endl;
      return false;
    }
    std::string tmpLine;
    while(infile >> tmpLine)
    {
      if(tmpLine == "#") // Stop at line with only #
      {
        break;
      }
      if(tmpLine.at(0) != '#') // Skip lines with # at the start
      {
        mInputImgPaths.push_back(tmpLine);
      }
    }
    infile.close();

    return true;
  }

  bool loadAVIFile(std::string videoPath)
  {
    if(videoPath == "")
    {
      return false;
    }

    return mVideoCap.open(videoPath);
  }

#ifndef _WINDOWS
  bool openCameraDev(std::string videoPath)
  {
    if(videoPath == "")
    {
      return false;
    }
    LIB_RESULT libResult = sdi::Initialize(0);
    if(libResult != LIB_SUCCESS)
    {
      std::cout << "SDI init failed!" << std::endl;
      return false;
    }
    std::cout << "SDI init success!" << std::endl;
    mSdiGrabber = std::unique_ptr<sdi_grabber>(new sdi_grabber);

    if(cameraType == sonyimx224) {
      libResult = mSdiGrabber->ProcessSet(gpGraph_mipi_simple, &gGraphMetadata_mipi_simple,
      kmem_srec_mipi_simple, sequencer_srec_mipi_simple);
    }
    else {
      libResult = mSdiGrabber->ProcessSet(gpGraph, &gGraphMetadata);
    }
    if(libResult != LIB_SUCCESS)
    {
      std::cout << "SDI grabber failed!" << std::endl;
      return false;
    }

    sdi_FdmaIO* lpSdiFdmaIo = (sdi_FdmaIO*)mSdiGrabber->IoGet(SEQ_OTHRIX_FDMA);
    SDI_ImageDescriptor lSdiImgDesc;
    if(cameraType == sonyimx224) {
      lSdiImgDesc = SDI_ImageDescriptor(mImageSensorWidthInPixels,
                                                          mImageSensorHeightInPixels,
                                                          RGB888);
    }
    else {
      lSdiImgDesc = SDI_ImageDescriptor(mImageSensorWidthInPixels,
                                                          mImageSensorHeightInPixels,
                                                          YUV422Stream_UYVY);
    }
    if(!lpSdiFdmaIo)
    {
      std::cout << "Null pointer" << std::endl;
      return false;
    }
    if(cameraType == sonyimx224) {
      libResult = lpSdiFdmaIo->DdrBufferDescSet(0, lSdiImgDesc);
    }
    else {
      libResult = lpSdiFdmaIo->DdrBufferDescSet(FDMA_IX_FastDMA_Out, lSdiImgDesc);
    }
    if(libResult != LIB_SUCCESS)
    {
      std::cout << "Buffer desc set failed!" << std::endl;
      return false;
    }

    const uint32_t bufferSize = 3;
    libResult = lpSdiFdmaIo->DdrBuffersAlloc(0, bufferSize);
    if(libResult != LIB_SUCCESS)
    {
      std::cout << "Buffer alloc failed!" << std::endl;
      return false;
    }

    libResult = mSdiGrabber->PreStart();
    if(libResult != LIB_SUCCESS)
    {
      std::cout << "Pre start failed!" << std::endl;
      return false;
    }

    for(uint32_t i = 0; i < 10000000; i++)
    {
      if(i % 1000000 == 0)
      {
        std::cout << ".";
      }
    }
    std::cout << std::endl;

    if(cameraType == sonyimx224) {
      SONY_Geometry_t lSonyGeometry;
      lSonyGeometry.mCsiIdx = CSI_IDX_0;
      SONY_GeometryGet(lSonyGeometry);
      lSonyGeometry.mVerFlip = true; //lWeFlipImageSensorVertically;
      lSonyGeometry.mHorFlip = true; //lWeFlipImageSensorHorizontally;
      SONY_GeometrySet(lSonyGeometry);
    }

    libResult = mSdiGrabber->Start();
    if(libResult != LIB_SUCCESS)
    {
      std::cout << "Image grabber failed!" << std::endl;
      return false;
    }

    mRect.width = mImageWidthInPixels;
    mRect.height = mImageHeightInPixels;
    mRect.x = (mImageSensorWidthInPixels - mImageWidthInPixels)/2;
    mRect.y = (mImageSensorHeightInPixels - mImageHeightInPixels)/2;

    return true;
  }

  void Mat2CVMat(vsdk::SMat& input, cv::Mat& output)
  {
    if(cameraType == sonyimx224) {
      output = cv::Mat(mRect.height, mRect.width, CV_8UC3);
      for(uint32_t h = 0; h < mRect.height; ++h)
      {
        const uint8_t* indata = input.ptr(h);
        uint8_t* outdata = output.ptr(h);
        memcpy((void*)outdata, (void*)indata, mRect.width*3);
      }
    }
    else {
      output = cv::Mat(mRect.height, mRect.width, CV_8UC2);
      for(uint32_t h = 0; h < mRect.height; ++h)
      {
        const uint8_t* indata = input.ptr(h);
        uint8_t* outdata = output.ptr(h);
        memcpy((void*)outdata, (void*)indata, mRect.width*2);
      }
    }
  }
#endif

  void initialize(const std::unordered_map<std::string, std::string>& aConfig) override
  {
    mLoop = GetParam(aConfig, "loop_input", "false") == "true";
    bool success = loadImagePaths(GetParam(aConfig, "file_reader_input"));
    if(!success)
    {
      success = loadAVIFile(GetParam(aConfig, "video_reader_input"));
      if(!success)
      {
#ifndef _WINDOWS
        if(GetParam(aConfig, "camera_type") == "ov10635") {
          cameraType = ov10635;
          mImageSensorHeightInPixels = 800;
          mImageHeightInPixels = 800;
        }
        else {
          cameraType = sonyimx224;
        }
        success = openCameraDev(GetParam(aConfig, "camera_reader_input"));
#endif
        if(!success)
        {
          ForceQuit();
          std::cout << "Bad init: no image paths or video path or camera dev" << std::endl;
        }
        else
        {
          mUseCamera = true;
        }
      }
      else
      {
        mUseVideo = true;
      }
    }
  }

  void process(size_t tid) override
  {
    bool success = true;
    std::vector<SSDDatagram> data(mInputQueue.size());
    // Blocks on data from input queue
    for(uint32_t i = 0; i < mInputQueue.size(); i++)
    {
      success = mInputQueue.at(i)->pop(data[i]);
      if(!data[i].valid)
      {
        success = false;
        break;
      }
    }

    auto startT = std::chrono::high_resolution_clock::now();
    if(success)
    {
      if(mUseVideo)
      {
        {
          std::lock_guard<std::mutex> llock(mMutex);
          success = mVideoCap.read(data[0].image);
        }
        if(mLoop && !success)
        {
          // Reset video stream if ended and loop flag is set
          {
            std::lock_guard<std::mutex> llock(mMutex);
            mVideoCap.set(CV_CAP_PROP_POS_FRAMES, 0);
            success = mVideoCap.read(data[0].image);
          }
        }
      }
      else if(mUseCamera)
      {
#ifndef _WINDOWS
        SDI_Frame lFrame;
        while(lFrame.mUMat.empty())
        {
          lFrame = mSdiGrabber->FramePopNonBlock(0);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        vsdk::SUMat lSensorRgb24_umat(lFrame.mUMat, mRect);
        vsdk::SMat sensor_vmat = lSensorRgb24_umat.getMat(vsdk::ACCESS_READ | OAL_USAGE_CACHED);
        Mat2CVMat(sensor_vmat, data[0].image);
        if(cameraType == ov10635) {
          // Convert CV mat from YCbCr422 to RGB 8 bit
          cvtColor(data[0].image, data[0].image, cv::COLOR_YUV2BGR_UYVY);
        }

        LIB_RESULT libResult = mSdiGrabber->FramePush(lFrame);
        if(libResult != LIB_SUCCESS)
        {
          success = false;
        }
#endif
      }
      else
      {
        std::string img_path = "";
        {
          std::lock_guard<std::mutex> llock(mMutex);
          if(mIdx < mInputImgPaths.size())
          {
            img_path = mInputImgPaths.at(mIdx++);
            if(mLoop)
            {
              mIdx %= mInputImgPaths.size();
            }
          }
        }
        
        std::cout << "Reading: " << img_path << std::endl;
        data[0].image = cv::imread(img_path);
        if(!data[0].image.data)
        {
          std::cout << "Error file not found, loop finished: " << img_path << std::endl;
          success = false;
          ForceQuit();
        }
      }
    }
    else
    {
      ForceQuit();
    }
    auto endT = std::chrono::high_resolution_clock::now();
    data[0].prof[0] = std::chrono::duration_cast<std::chrono::milliseconds>(endT - startT).count();

    // Pushes data to output queue
    data[0].valid = success;
    mOutputQueue.at(0)->push(data[0]);
    for(uint32_t i = 1; i < mOutputQueue.size(); i++)
    {
      data[i].valid = success;
      //duplicate to have img to draw output onto
      if(success)
      {
        data[i].image = data[0].image.clone();
      }
      data[i].prof[0] = 0;
      mOutputQueue.at(i)->push(data[i]);
    }
  }
};

/*!****************************************************************************
* \ingroup ThreadingAPI
* \brief Implements preprocessing stage of MSSD
******************************************************************************/
class SSDPreProc : public MultiThread<SSDDatagram, SSDDatagram>
{
public:
  SSDPreProc(std::string aPoolName, size_t aThreadCount) : MultiThread(aPoolName, aThreadCount)
  {
  }

private:
  void initialize(const std::unordered_map<std::string, std::string>& aConfig) override
  {
    //Do nothing
  }

  void process(size_t tid) override
  {
    bool success = true;
    std::vector<SSDDatagram> data(mInputQueue.size());
    // Blocks on data from input queue
    for(uint32_t i = 0; i < mInputQueue.size(); i++)
    {
      success = mInputQueue.at(i)->pop(data[i]);
      if(!data[i].valid)
      {
        success = false;
        break;
      }
    }

    auto startT = std::chrono::high_resolution_clock::now();
    //Check image and input tensor is valid
    if(success)
    {
      //Copy and resize image to tensor
      resizeBilinearAndNormalize(data[0].image, data[0].inTensor, {128}, {1.0});
    }
    else
    {
      ForceQuit();
    }
    auto endT = std::chrono::high_resolution_clock::now();
    data[0].prof[1] = std::chrono::duration_cast<std::chrono::milliseconds>(endT - startT).count();

    // Pushes data to output queue
    data[0].valid = success;
    mOutputQueue.at(0)->push(data[0]);
    for(uint32_t i = 1; i < mOutputQueue.size(); i++)
    {
      data[i].valid = success;
      data[i].inTensor->CopyDataFrom(*data[0].inTensor);
      data[i].prof[1] = 0;
      mOutputQueue.at(i)->push(data[i]);
    }
  }
};

/*!****************************************************************************
* \ingroup ThreadingAPI
* \brief Implements processing stage of MSSD
******************************************************************************/
class SSDProc : public MultiThread<SSDDatagram, SSDDatagram>
{
public:
  SSDProc(std::string aPoolName, size_t aThreadCount) : MultiThread(aPoolName, aThreadCount)
  {

  }

  void Init(std::vector<std::shared_ptr<GraphRunner>> aGraphRunners)
  {
    for(auto & gr : aGraphRunners)
    {
      mGraphRunners.push_back(gr);
    }
  }

private:
  //TODO: pass in vector of shared pointers
  //std::vector<GraphRunner> mGraphRunners;
  std::vector<std::shared_ptr<GraphRunner>> mGraphRunners;

  void initialize(const std::unordered_map<std::string, std::string>& aConfig) override
  {
    if(mGraphRunners.size() != ThreadCount())
    {
      std::cout << "Thread mismatch!" << std::endl;
      ForceQuit();
    }
  }

  void process(size_t tid) override
  {
    SSDDatagram data;
    bool success = true;
    if(mInputQueue.size() == 1)
    {
      success |= mInputQueue.at(0)->pop(data);
    }
    else
    {
      success |= mInputQueue.at(tid)->pop(data);
    }

    auto startT = std::chrono::high_resolution_clock::now();
    if(data.valid && success)
    {
      //Check tensor is valid
      Tensor* tmpInTensor   = mGraphRunners.at(tid)->mInputTensors.at(0);
      Tensor* tmpOutTensor0 = mGraphRunners.at(tid)->mOutputTensors.at(0).at(0);
      Tensor* tmpOutTensor1 = mGraphRunners.at(tid)->mOutputTensors.at(0).at(1);

      // Set input tensor to use tensor from queue
      tmpInTensor->SetParent(data.inTensor, TensorShapeBase(0, 0, 0, 0));
      // Set output tensor to use buffer from queue
      tmpOutTensor0->SetParent(data.outTensor0, TensorShapeBase(0, 0, 0, 0)); //concat
      tmpOutTensor1->SetParent(data.outTensor1, TensorShapeBase(0, 0, 0, 0)); //concat_1

      // Run graph
      Status_t lStatus = mGraphRunners.at(tid)->RunGraph();
      if(lStatus != Status_t::SUCCESS)
      {
        std::cout << "Bad run!" << std::endl;
        data.valid = false;
      }
    }
  
    data.valid = data.valid && success;
    if(!data.valid)
    {
      ForceQuit();
    }

    auto endT = std::chrono::high_resolution_clock::now();
    data.prof[2] = std::chrono::duration_cast<std::chrono::milliseconds>(endT - startT).count();

    if(mOutputQueue.size() == 1)
    {
      mOutputQueue.at(0)->push(data);
    }
    else
    {
      mOutputQueue.at(tid)->push(data);
    }
  }
};

static std::string trackingModeDisp[4] = {"None", "Kalman", "Heatmap", "Heatmap + Kalman"};
static Tracker tracker;

/*!****************************************************************************
* \ingroup ThreadingAPI
* \brief Implements postprocessing stage of MSSD
******************************************************************************/
class SSDPostProc : public MultiThread<SSDDatagram, SSDDatagram>
{
public:
  SSDPostProc(std::string aPoolName, size_t aThreadCount) : MultiThread(aPoolName, aThreadCount)
  {
  }

private:
  std::vector<ODPostProcSSD> mPostProcSSD;
  std::vector<std::string>   mClassLabels;
  bool                       mPrintResults = false;
  int mClassNum = 0;
  float mIoUThresh = 0;
  float mScoreThresh = 0;
  // Tracker tracker;
  // KalmanTracker kalmanTracker;

  void initialize(const std::unordered_map<std::string, std::string>& aConfig) override
  {
    tracker.configure(aConfig);
    // kalmanTracker.configure(aConfig);
    mIoUThresh = std::stof(GetParam(aConfig, "iou_thresh", "0.6"));
    mScoreThresh = std::stof(GetParam(aConfig, "score_thresh", "0.35"));
    mPrintResults = GetParam(aConfig, "print_results") == "true";

    uint32_t lThreads = ThreadCount();
    // Create a post proc object for each thread
    for(uint32_t i = 0; i < lThreads; i++)
    {
      mPostProcSSD.emplace_back(-1);
    }

    std::string box_config  = GetParam(aConfig, "post_proc_config");
    std::string box_config1  = GetParam(aConfig, "post_proc_config1");
    if(box_config != "")
    {
      if(box_config1 == "")
      {
        for(uint32_t i = 0; i < lThreads; i++)
        {
          mPostProcSSD.at(i).LoadBoxes(box_config, BoxFormat::PARAM);
        }
      }
      else
      {
        mPostProcSSD.at(0).LoadBoxes(box_config, BoxFormat::PARAM);
        mPostProcSSD.emplace_back(-1);
        mPostProcSSD.at(1).LoadBoxes(box_config1, BoxFormat::PARAM);
      }
    }
    else
    {
      ForceQuit();
      std::cout << "Please provide default anchor box params!" << std::endl;
    }

    //Parse class labels
    std::string aLabelsFilepath = GetParam(aConfig, "class_label_file");
    LoadTxtFile(aLabelsFilepath, mClassLabels);
  }

  void process(size_t tid) override
  {
    bool success = true;
    std::vector<SSDDatagram> data(mInputQueue.size());

    for(uint32_t i = 0; i < mInputQueue.size(); i++)
    {
      // Blocks on data from input queue
      success = mInputQueue.at(i)->pop(data[i]);

      auto startT = std::chrono::high_resolution_clock::now();
      if(data[i].valid && success)
      {
        if(data.size() > 1)
        {
          tid = i;
        }

        Status_t lStatus = mPostProcSSD.at(tid).DecodeBoxes(*(data[i].outTensor0));
        if(lStatus != Status_t::SUCCESS)
        {
          std::cout << "Bad decode!" << std::endl;
          success = false;
        }

        lStatus = mPostProcSSD.at(tid).NonMaxSupression(*(data[i].outTensor1),
            mIoUThresh, mScoreThresh, data[i].image.rows, data[i].image.cols);
        if(lStatus != Status_t::SUCCESS)
        {
          std::cout << "Bad non-max suppression!" << std::endl;
          success = false;
        }
      }
      else
      {
        success = false;
      }

      if(success)
      {
        std::vector<std::pair<uint32_t, float32_t>> results;
        cv::Scalar colour = cv::Scalar(0, 250, 50);
        if(tid == 1)
        {
          colour = CV_RGB(255, 160, 0);
        }
        bool randColour = false;
        if(data.size() == 1)
        {
          randColour = true;
        }
        auto bb = mPostProcSSD.at(tid).GetBoundingBoxes();
        tracker.setHeatmapSize(data[i].image.rows, data[i].image.cols);
        bb = tracker.update(bb);
        DrawColouredBoundingBoxes(data[i].image, bb, mClassLabels, results, colour, randColour);
        if(mPrintResults)
        {
          mPostProcSSD.at(tid).ShowBoxesAndScores();
        }
      }
      else
      {
        ForceQuit();
      }
      auto endT = std::chrono::high_resolution_clock::now();
      data[i].prof[3] = std::chrono::duration_cast<std::chrono::milliseconds>(endT - startT).count();
    }

    // Pushes data to output queue
    data[0].valid = success;
    mOutputQueue.at(0)->push(data[0]);
    for(uint32_t i = 1; i < mOutputQueue.size(); i++)
    {
      data[i].valid = success;
      mOutputQueue.at(i)->push(data[i]);
    }
  }
};


/*!****************************************************************************
* \ingroup ThreadingAPI
* \brief Implements display of SSD results
******************************************************************************/
class ImageDisplay : public MultiThread<SSDDatagram, SSDDatagram>
{
public:
  ImageDisplay(std::string aPoolName, size_t aThreadCount) : MultiThread(aPoolName, aThreadCount)
  {
  }
  ~ImageDisplay()
  {
    if(mSaveVideo)
    {
      mVideoWriter.release();
    }
  }

private:
#ifndef _WINDOWS
  io_FrameOutput_t mDisplayOutput;
#endif

  int  mDispDelay     = 10;
  int  mDispImgWidth  = 1280;
  int  mDispImgHeight = 720;
  int  mProfInterval  = 10;
  bool mFlipImg = false;
  bool mShowLCD = false;

  cv::VideoWriter mVideoWriter;
  bool mSaveVideo = false;
  bool mPutLogo = false;
  bool mShowProf = false;

  size_t mLastProcTime = 0;
  size_t mEncodeTime = 0;
  std::mutex mMutex;


  void initialize(const std::unordered_map<std::string, std::string>& aConfig) override
  {
    mDispDelay = std::stoi(GetParam(aConfig, "display_delay", "0"));
    mDispImgWidth = std::stoi(GetParam(aConfig, "display_width", "1280"));
    mDispImgHeight = std::stoi(GetParam(aConfig, "display_height", "720"));
    if(GetParam(aConfig, "flip_image") == "true")
    {
      mFlipImg = true;
    }
    else
    {
      mFlipImg = false;
    }
    mShowLCD = GetParam(aConfig, "show_lcd", "true") == "true";
    mSaveVideo = GetParam(aConfig, "save_video", "false") == "true";
    mPutLogo = GetParam(aConfig, "put_logo", "true") == "true";
    mProfInterval = std::stoi(GetParam(aConfig, "interval_profiling", "10"));
    mShowProf = GetParam(aConfig, "show_prof", "true") == "true";

    std::string videoFile = GetParam(aConfig, "save_video_path", "output_video.avi");
    if(mSaveVideo)
    {
      int fps = std::stoi(GetParam(aConfig, "fps", "30"));
      mVideoWriter.open(videoFile, CV_FOURCC('D','I','V','X'), fps, cv::Size(mDispImgWidth, mDispImgHeight));
    }

#ifndef _WINDOWS
  mDisplayOutput.Init(mDispImgWidth, mDispImgHeight, io::IO_DATA_DEPTH_08, io::IO_DATA_CH3);
#endif
  }

  void ConcatMat(cv::Mat& input0, cv::Mat& input1, cv::Mat& output)
  {
    output = cv::Mat(input0.rows, input0.cols+input1.cols, CV_8UC3);
    for(uint32_t h = 0; h < input0.rows; ++h)
    {
      uint8_t* indata0 = input0.ptr(h);
      uint8_t* indata1 = input1.ptr(h);
      uint8_t* outdata0 = output.ptr(h);
      uint8_t* outdata1 = outdata0 + input0.cols*3;
      memcpy((void*)outdata0, (void*)indata0, input0.cols*3);
      memcpy((void*)outdata1, (void*)indata1, input1.cols*3);
    }
  }

  void PutLogo(cv::Mat& input, int offsetY, int offsetX)
  {
    uint32_t pLogoPixel = 0;
    for (int i = 0; i < NXP_LOGO_HEIGHT; i++)
    {
      char* row = (char*)input.ptr(offsetY+i);
      for (int j = 0; j < (NXP_LOGO_WIDTH); j++)
      {
        char* pPixel = row + (offsetX+j)*3;
        if ((nxp_logo[pLogoPixel + 2] != 0xff) ||
            (nxp_logo[pLogoPixel + 1] != 0xff) ||
            (nxp_logo[pLogoPixel] != 0xff))
        {
          pPixel[0] = nxp_logo[pLogoPixel + 2];
          pPixel[1] = nxp_logo[pLogoPixel + 1];
          pPixel[2] = nxp_logo[pLogoPixel];
        }
        pLogoPixel += 3;
      }
    }
  }


  void PutLogo_pic(cv::Mat& input, int offsetY, int offsetX)
  {

  // read wpi_logo
  cv::Mat logo = cv::imread("logo_wpi.jpg");
  if (!logo.data)
  {
	  std::cout << "wpi log load filed" << std::endl;
	 // return -1;
  }


	  //------add wpi logo------
	  cv::Mat imageROI = input(cv::Rect(input.cols - logo.cols, input.rows - logo.rows, logo.cols, logo.rows)); 

	  cv::addWeighted(imageROI, 0.2, logo, 0.8, 0.0, imageROI);
	  //-----end add wpi logo------
  }





  void process(size_t tid) override
  {
    bool success = true;
    std::vector<SSDDatagram> data(mInputQueue.size());
    // Blocks on data from input queue
    for(uint32_t i = 0; i < mInputQueue.size(); i++)
    {
      success = mInputQueue.at(i)->pop(data[i]);
      if(!data[i].valid)
      {
        success = false;
        break;
      }
    }

    auto startT = std::chrono::high_resolution_clock::now();
#ifndef _WINDOWS
    // Grab stats
    if(success)
    {
      size_t procItems = 0;
      size_t runTime = 0;
      double frame_rate = 0;
      if(GetProfilingFlag())
      {
        procItems = GetProcItems();
        runTime = GetRunTime();
        frame_rate = 1000*((double)procItems/(double)(runTime));
        if(mProfInterval != -1 && procItems % mProfInterval == 0)
        {
          std::cout << "App frame rate: " << frame_rate << ", items: " << procItems << " / " << runTime << std::endl;
        }
      }
      cv::Mat dispBuffer = cv::Mat(mDispImgWidth, mDispImgHeight, CV_8UC3);
      if(data.size() == 1)
      {
        // Use profiling info from last frame
        data[0].prof[4] = mLastProcTime;
        // Resize display image
        cv::resize(data[0].image, dispBuffer, cv::Size(mDispImgWidth, mDispImgHeight));
        // Add profiling info
        if(!mSaveVideo && mShowProf)
        {
          cv::putText(dispBuffer, "[FPS]    : " + std::to_string(frame_rate).substr(0, 4), cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX, 0.8, CV_RGB(250, 10, 10), 2);
          cv::putText(dispBuffer, "[Frame]  : " + std::to_string(procItems), cv::Point(50, 90), cv::FONT_HERSHEY_SIMPLEX, 0.8, CV_RGB(250, 10, 10), 2);
          cv::putText(dispBuffer, "[Res]    : " + std::to_string(mDispImgWidth) + "x" + std::to_string(mDispImgHeight), cv::Point(50, 130), cv::FONT_HERSHEY_SIMPLEX, 0.8, CV_RGB(250, 10, 10), 2);
          cv::putText(dispBuffer, "[Track]  : " + trackingModeDisp[tracker.getTrackingMode()], cv::Point(50, 170), cv::FONT_HERSHEY_SIMPLEX, 0.8, CV_RGB(250, 10, 10), 2);
          for(int i = 0; i < 5; i++)
          {
          cv::putText(dispBuffer, "[Proc]   : " + std::to_string(data[0].prof[i]) + " ms", cv::Point(50,210+20*i), cv::FONT_HERSHEY_SIMPLEX, 0.4, CV_RGB(250, 10, 10), 1);
          }
        }
      }
      else if(data.size() == 2)
      {
        // Use profiling info from last frame
        data[0].prof[4] = mLastProcTime;
        data[1].prof[4] = mLastProcTime;
        // Resize and Merge images
        cv::putText(data[0].image, "network1", cv::Point(50,700), cv::FONT_HERSHEY_SIMPLEX, 0.7, CV_RGB(250, 10, 10), 2);
        cv::putText(data[1].image, "network2", cv::Point(50,700), cv::FONT_HERSHEY_SIMPLEX, 0.7, CV_RGB(250, 10, 10), 2);
        cv::Mat resizeBuffer, resizeBuffer1;
        cv::resize(data[0].image, resizeBuffer, cv::Size(mDispImgWidth/2, mDispImgHeight));
        cv::resize(data[1].image, resizeBuffer1, cv::Size(mDispImgWidth/2, mDispImgHeight));
        ConcatMat(resizeBuffer, resizeBuffer1, dispBuffer);
        // Add profiling info
        if(!mSaveVideo && mShowProf)
        {
          cv::putText(dispBuffer, "[FPS]    : " + std::to_string(frame_rate).substr(0, 4), cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX, 0.8, CV_RGB(250, 10, 10), 2);
          cv::putText(dispBuffer, "[Frame]  : " + std::to_string(procItems), cv::Point(50, 90), cv::FONT_HERSHEY_SIMPLEX, 0.8, CV_RGB(250, 10, 10), 2);
          cv::putText(dispBuffer, "[Res]    : " + std::to_string(mDispImgWidth) + "x" + std::to_string(mDispImgHeight), cv::Point(50, 130), cv::FONT_HERSHEY_SIMPLEX, 0.8, CV_RGB(250, 10, 10), 2);
          for(int i = 0; i < 5; i++)
          {
          cv::putText(dispBuffer, "[Proc0]   : " + std::to_string(data[0].prof[i]) + " ms", cv::Point(50,170+40*i), cv::FONT_HERSHEY_SIMPLEX, 0.4, CV_RGB(250, 10, 10), 1);
          cv::putText(dispBuffer, "[Proc1]   : " + std::to_string(data[1].prof[i]) + " ms", cv::Point(50,190+40*i), cv::FONT_HERSHEY_SIMPLEX, 0.4, CV_RGB(250, 10, 10), 1);
          }
        }
      }
      else
      {
        data[0].valid = false;
        success = false;
        ForceQuit();
      }
      if(success)
      {
        if(mPutLogo)
        {
          //PutLogo(dispBuffer, mDispImgHeight - 125, mDispImgWidth - 225);
          PutLogo_pic(dispBuffer, mDispImgHeight - 125, mDispImgWidth - 225);
        }
        if(mFlipImg)
        {
          cv::flip(dispBuffer, dispBuffer, -1);
        }
        if(mSaveVideo)
        {
          std::lock_guard<std::mutex> llock(mMutex);
          mVideoWriter.write(dispBuffer);
        }
        if(mShowLCD)
        {
          std::lock_guard<std::mutex> llock(mMutex);
          mDisplayOutput.PutFrame(dispBuffer.data, false);
        }
        if(mDispDelay != 0)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(mDispDelay));
        }
      }
    }
    else
    {
      ForceQuit();
    }
#endif
    auto endT = std::chrono::high_resolution_clock::now();
    mLastProcTime = std::chrono::duration_cast<std::chrono::milliseconds>(endT - startT).count();

    // Pushes data to output queue
    data[0].valid = success;
    mOutputQueue.at(0)->push(data[0]);
    for(uint32_t i = 1; i < mOutputQueue.size(); i++)
    {
      data[i].valid = success;
      mOutputQueue.at(i)->push(data[i]);
    }
  }
};

} //namespace airunner

#endif
