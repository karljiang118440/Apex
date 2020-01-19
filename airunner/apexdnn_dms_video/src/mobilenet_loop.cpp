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



#include "test_cases.hpp"
#include <airunner_postprocessing_ssd.hpp>
#include "test_cases.hpp"
#include<read_image.hpp>
#include <opencv2/opencv.hpp>  
#include "common_time_measure.h"
#include <oal.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>



using namespace airunner;

inline
static void stopwatch(bool start, std::string verb = "")
{
  static auto startTime = std::chrono::high_resolution_clock::now();

  if(start){
    startTime = std::chrono::high_resolution_clock::now();
  }
  else{
    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken to " << verb << ": " 
      << std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count() 
      << " nanoseconds" 
      << std::endl;
  }
}


template<typename T> inline std::vector<std::pair<int, float>> processResults(T* aResults, int aNumResults)
{
  // Convert results to key-pair list
  std::vector<std::pair<int, float>> resultPairs;
  
  for(int i = 0; i < aNumResults; i++) {
    std::pair<int, T> pair(i, (float)(aResults[i]));
    resultPairs.push_back(pair);
  }
  std::sort(resultPairs.begin(), resultPairs.end(), [](std::pair<int, T> left, std::pair<int, T> right) {
    return left.second > right.second;
  });

  return resultPairs;
}

int mobilenet_loop(const std::string& aMnetGraph, const std::string& aResultGraph, const std::string& aDescriptionFile, const std::string& aSlimLabelsFile)
{


  Status_t status;

  // Creating a new workspace  
  TargetInfo lRefInfo = TargetInfo();
  ApexTargetInfo lApexInfo = ApexTargetInfo();
  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::map<std::string, TargetInfo*>{
      {target::REF, &lRefInfo}, {target::APEX, &lApexInfo}}));

  // Create and populate the Graphs
  auto net_mobile = std::unique_ptr<Graph>(new Graph (lWorkspace.get())); 
  auto netFixedToFloat = std::unique_ptr<Graph>(new Graph (lWorkspace.get()));

  Tensor* lApexNetInput  = net_mobile->AddTensor(std::unique_ptr<Tensor>(Tensor::Create<>(
                        "NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
                        TensorShape<TensorFormat_t::NHWC>{1, 224, 224,3},
                        TensorLayout<TensorFormat_t::NHWC>())));

  lApexNetInput->SetQuantParams({QuantInfo(-1, 0)});
  lApexNetInput->Allocate(Allocation_t::OAL);
  



// karl.jiang-20200119***********
  // add opencv parameters

   cv::Mat outputImage;

	 cv::VideoCapture capture; /* open video */
	 capture.open("./data/driver.avi");
	 
	   if(!capture.isOpened()) // if not success, exit program
	   {
		 printf("Cannot open the video file: \n");
		//return -1;
	   } // if video open failed
	 
	 outputImage = cv::Mat::zeros(cv::Size(1280, 720), CV_8UC3);


		while(true)
		{	
	  
 
		   capture >> outputImage;		
			if (!capture.read(outputImage))
			  {
			  printf("\nread video frame failed!\n"); /* capture opencv frame */
			  continue;
			  }
	  
	  
			if(!outputImage.data)
			{
			  std::cout << "Mobilenet: could not open or find image" << std::endl;
			  return {};
			}
    }
// outputImage --> lApexNetInput

			stopwatch(true);
			resizeBilinearAndNormalize(outputImage, lApexNetInput, true, {128}, 1.0f);
			stopwatch(false, "resizeBilinearAndNormalize");
	  

//************* karl.jiang-20200119









  // Integer output of the aMnetGraph except the last 3 layers. 
  std::vector<Tensor*> fixedOutput;
  
  status = LoadNetFromTensorFlow(*net_mobile, aMnetGraph, lApexNetInput, fixedOutput);
  if(Status_t::SUCCESS != status || fixedOutput.empty()){
    std::cout << "Failed to load net" << std::endl;
    return -1;
  }

#ifndef _WINDOWS
  status = net_mobile->SetTargetHint(target::APEX);
#else
  status = net_mobile->SetTargetHint(target::REF);
#endif

  if(Status_t::SUCCESS != status){
    std::cout << "Set APEX target failed" << std::endl;
    return -1;
  }
  
  status = net_mobile->Prepare(); 
  
  if (Status_t::SUCCESS != status){
    std::cout << "APEX net verification failed" << std::endl;
    return -1;
  }

  int dimN = fixedOutput[0]->Dim(3);
  int dimH = fixedOutput[0]->Dim(2);
  int dimW = fixedOutput[0]->Dim(1);
  int dimC = fixedOutput[0]->Dim(0);
  
  Tensor* floatInterm  = netFixedToFloat->AddTensor(std::unique_ptr<Tensor>(Tensor::Create<>(
                        "FLOAT_INTERM_TENSOR", DataType_t::FLOAT,
                        TensorShape<TensorFormat_t::NHWC>{dimN, dimH,dimW, dimC},
                        TensorLayout<TensorFormat_t::NHWC>())));
  
  floatInterm->Allocate(Allocation_t::HEAP);
  
  // Final Output.
  std::vector<Tensor*> Output;
  status = LoadNetFromTensorFlow(*netFixedToFloat, aResultGraph, floatInterm, Output);
  if(Status_t::SUCCESS != status || Output.empty()){
    std::cout << "Failed to load net" << std::endl;
    return -1;
  }

  status = netFixedToFloat->SetTargetHint(target::REF);
  if(Status_t::SUCCESS != status){
    std::cout << "Set APEX target failed" << std::endl;
    return -1;
  }

  status = netFixedToFloat->Prepare(); 
  if (Status_t::SUCCESS != status){
    std::cout << "APEX net verification failed" << std::endl;
    return -1;
  }

  // Load class labels
  std::ifstream labelfile(aSlimLabelsFile);
  std::vector<std::string> classLabels;
  std::string line;
  
  while(std::getline(labelfile, line)){
      line.pop_back();
      classLabels.push_back(line);
  }

  std::string imagePath;
  
  struct Normalize norm = { 128, 128 };

#ifndef _WINDOWS
  io_FrameOutput_t lFrameOutput;
  lFrameOutput.Init(DISPLAY_SCENE_WIDTH, DISPLAY_SCENE_HEIGHT, io::IO_DATA_DEPTH_08, io::IO_DATA_CH3);
#endif

  std::unique_ptr<float[]> s_result(new float[1001]);
  float* result = s_result.get();
  
  while(true)
  {
    std::ifstream infile(aDescriptionFile);

    while(infile >> imagePath)
    {
      std::cout << "Classifying: " << imagePath << std::endl;

      if (-1 == ReadImageToTensor(imagePath, lApexNetInput, nullptr, 128, 128, 0))
      {
        std::cout << "Failed to read: " << imagePath << std::endl;
        return -1;
      }
      lApexNetInput->Flush();
      fixedOutput[0]->Invalidate();

      // Run image through parsed model
      
      status = net_mobile->Run();
      if(status != Status_t::SUCCESS)
      {
        std::cout << "Net execution failed" << std::endl;
         
        return -1;
      }

      status = floatInterm->CopyDataFrom(*fixedOutput[0]);
      if(status != Status_t::SUCCESS)
      {
        std::cout << "Copy failed" << std::endl;
         
        return -1;
      }
      
      status = netFixedToFloat->Run();
      if(status != Status_t::SUCCESS)
      {
        std::cout << "Fixed to float net execution failed" << std::endl;
         
        return -1;
      }

      int numClasses =  Output[0]-> Dim(0) *
                        Output[0]-> Dim(1) *
                        Output[0]-> Dim(2) * 
                        Output[0]-> Dim(3);

      softmax(Output[0], result);

      // Get top 5 result
      auto results = processResults(result, numClasses);

      std::cout << "Top 5: " << std::endl;
      std::cout << std::right << std::setw(20) << classLabels[results[0].first] << ", " << std::to_string(results[0].second) << std::endl;
      std::cout << std::right << std::setw(20) << classLabels[results[1].first] << ", " << std::to_string(results[1].second) << std::endl;
      std::cout << std::right << std::setw(20) << classLabels[results[2].first] << ", " << std::to_string(results[2].second) << std::endl;
      std::cout << std::right << std::setw(20) << classLabels[results[3].first] << ", " << std::to_string(results[3].second) << std::endl;
      std::cout << std::right << std::setw(20) << classLabels[results[4].first] << ", " << std::to_string(results[4].second) << std::endl;
      std::cout << std::endl;
#ifndef _WINDOWS
      DisplayImage(lFrameOutput, imagePath, classLabels, results); 
      sleep(1);
#endif
    }
  }

  return 0;
}
