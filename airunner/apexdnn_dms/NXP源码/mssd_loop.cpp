/*****************************************************************************
 *
 * NXP Confidential Proprietary
 *
 * Copyright (c) 2018 NXP Semiconductor;
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

#include <postprocessing_ssd.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <time.h>


#include "test_cases.hpp"
//#define video_airunner

using Clock = std::chrono::high_resolution_clock;
using namespace airunner;

namespace airunner { namespace internal {

std::string DebugDumpTensorQuantized(const Tensor& aA);

} }

#ifdef cam_airunner
extern ISPCameraInputStereo ai_camerainput;
SDI_Frame lFrameRgb;
#endif


int mssd_apex_loop(const std::string& aMssdGraph,
                   const std::string& aDescriptionFile,
                   const std::string& aLabels,
                   int numClasses, 
                   int anchorGenVer)
{


  std::cout << "Running MSSD graph for APEX" << std::endl;

  Status_t status;

  numClasses = 14;

  // Load MSSD graph
  stopwatch(true);
  TargetInfo     lRefInfo  = TargetInfo();
  ApexTargetInfo lApexInfo = ApexTargetInfo();
  auto           lWorkspace =
      std::unique_ptr<Workspace>(new Workspace(std::map<std::string, TargetInfo*>{
          {target::REF, &lRefInfo}, {target::APEX, &lApexInfo}}));

  auto net_mssd = std::unique_ptr<Graph>(new Graph(lWorkspace.get()));

#ifndef _WINDOWS
  Tensor* lApexNetInput = net_mssd->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("APEX_NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
                       TensorShape<TensorFormat_t::NHWC>{1, 360, 480, 3}, 
                       TensorLayout<TensorFormat_t::NHWC>())));
#else
  Tensor* lApexNetInput = net_mssd->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("APEX_NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
          TensorShape<TensorFormat_t::NHWC>{1, 300, 300, 3},
          TensorLayout<TensorFormat_t::NHWC>())));
#endif

  lApexNetInput->SetQuantParams({QuantInfo(-1, 0)});
  lApexNetInput->Allocate(Allocation_t::OAL);

  auto input = std::unique_ptr<Tensor>(new Tensor("TMP_INPUT"));

  // Integer output of the aMssdGraph
  std::vector<Tensor*> output;
  status = LoadNetFromTensorFlow(*net_mssd, aMssdGraph, lApexNetInput, output);
  if(Status_t::SUCCESS != status || output.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return -1;
  }

#if 0 
  Tensor* boxOutputFixedPoint = net_mssd->GetTensor("concat"); 
  Tensor* classOutputFixedPoint = net_mssd->GetTensor("concat_1"); 
#else 
  Tensor* boxOutputFixedPoint = net_mssd->GetTensor("concat_1"); 
  Tensor* classOutputFixedPoint = net_mssd->GetTensor("concat"); 
#endif

  if(nullptr == boxOutputFixedPoint || nullptr == classOutputFixedPoint)
  {
    std::cout << "Net output box/class not found " << std::endl;
    return -1;
  }

#ifndef _WINDOWS
  status = net_mssd->SetTargetHint(target::APEX);
#else
  status = net_mssd->SetTargetHint(target::REF);
#endif

  if(Status_t::SUCCESS != status)
  {
    std::cout << "Set APEX target failed" << std::endl;
    return -1;
  }

  status = net_mssd->Prepare();

  if(Status_t::SUCCESS != status)
  {
    std::cout << "APEX net verification failed" << std::endl;
    return -1;
  }


#ifndef _WINDOWS
  io_FrameOutput_t lFrameOutput;
  lFrameOutput.Init(DISPLAY_SCENE_WIDTH, DISPLAY_SCENE_HEIGHT, io::IO_DATA_DEPTH_08,
                    io::IO_DATA_CH3);
#endif

  std::ifstream            labelfile(aLabels);
  std::vector<std::string> classLabels;
  std::string              line;
 // const char	 mssd_anchorboxini[]	= "boxes_ssd_new.txt";

  while(std::getline(labelfile, line))
  {
    classLabels.push_back(line);
  }
  cv::Mat outputImage;
  stopwatch(false, "Start Up");

  
  ODPostProcSSD lPostProcSSD(numClasses, 10.f, 10.f, 5.f, 5.f);
  status = lPostProcSSD.LoadBoxes("boxes_ssd_neusoft_480x360.txt",BoxFormat::PARAM);
  
  //status = lPostProcSSD.CreateDefaultAnchors(anchorGenVer-1); 

#ifdef video_airunner 
 cv::VideoCapture capture; /* open video */
   capture.open("test000.avi");
 
   if(!capture.isOpened()) // if not success, exit program
   {
	 printf("Cannot open the video file: \n");
	//return -1;
   } // if video open failed
 
 outputImage = cv::Mat::zeros(cv::Size(1280, 720), CV_8UC3);
#endif
  
  while(true)
  {
   #ifndef cam_airunner
    std::ifstream infile(aDescriptionFile);
    std::string   imagePath;

    while(infile >> imagePath)
  #endif
    {
      stopwatch(true);
	  #ifdef video_airunner
	  
	  if (!capture.read(outputImage))
	  	{
	  	printf("\nread video frame failed!\n"); /* capture opencv frame */
		capture.set(1,0);
		continue;
	  	}
	  #endif

	  #ifdef cam_airunner
	  // Get Frames
				 ai_camerainput.GetFrame(FDMA_IX_RGB888, lFrameRgb);
				// ai_camerainput.GetFrame(FDMA_IX_G_SUB_2, lFrameRgb);
				 if(lFrameRgb.mUMat.empty())
				 {
				   printf("Failed to pop the RGB frame.\n");
				   continue;
				 }
			  outputImage = lFrameRgb.mUMat.getMat(vsdk::ACCESS_RW | OAL_USAGE_CACHED);
			  if(!outputImage.data)
			  {
				std::cout << "ReadImageToTensor: Could not open or find image" << std::endl;
				return {};
			  }
	  #endif
	  
      #ifndef cam_airunner
	  std::cout << "Detecting objects for: " << imagePath << std::endl;
      outputImage = cv::imread(imagePath);	
	  
	  #endif
      stopwatch(false, "Imread");
      if(!outputImage.data)
      {
        std::cout << "SSD: could not open or find image" << std::endl;
        return {};
      }
      stopwatch(true);
      resizeBilinearAndNormalize(outputImage, lApexNetInput, true, {128}, 1.0f);
      stopwatch(false, "Resize");

      stopwatch(true);
	  lApexNetInput->Flush();
      output[0]->Invalidate();
      output[1]->Invalidate();

      // 2. Run preprocessed img through MSSD to produce raw class and box prediction results
      status = net_mssd->Run();
      stopwatch(false, "Run Graph");
      if(Status_t::SUCCESS != status)
      {
        std::cout << "APEX net verification failed" << std::endl;
        return -1;
      }

	  stopwatch(true);
	  

      lPostProcSSD.DecodeBoxes(*boxOutputFixedPoint);
      lPostProcSSD.NonMaxSupression(*classOutputFixedPoint, 0.6, 0.35, outputImage.rows, outputImage.cols);
      stopwatch(false, "Postproc");
	  
	  stopwatch(true);
	  lPostProcSSD.ShowBoxesAndScores();

      std::vector< std::pair<int, float> > results;   
      DrawBoundingBoxes(lPostProcSSD.GetBoundingBoxes(), outputImage, results);

#ifdef _WINDOWS
      cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);// Create a window for display.
      cv::imshow("Display window", outImage);                // Show our image inside it.
      cv::waitKey(2000);
#else
      DisplayImageOD(lFrameOutput, outputImage, classLabels, results);
	  stopwatch(false, "DisplayImageOD");
	  #ifdef cam_airunner
		if(ISPCameraInputStereo::sStop)
	    {
	      printf("*** STOP ***\n");
	      break; // break if Ctrl+C pressed
	    }        // if Ctrl+C
	//  #elif video_airunner
	 // ;
	  #else
        sleep(2);
	  #endif
#endif
    
    }

	
  }



  #ifdef cam_airunner
  //*** Stop ISP processing ***
  ai_camerainput.Stop();
  ai_camerainput.Deinit();
  #endif


  return 0;
}


