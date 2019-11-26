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

#include <airunner_postprocessing_ssd.hpp>
#include "test_cases.hpp"


#include "common_time_measure.h"

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





using Clock = std::chrono::high_resolution_clock;
using namespace airunner;

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
              << std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count()
              << " nanoseconds" << std::endl;
  }
}

int mssd_apex_loop(const std::string& aMssdGraph,
                   const std::string& aDescriptionFile,
                   const std::string& aLabels,
                   int numClasses, 
                   int anchorGenVer)
{


  std::cout << "Running MSSD graph for APEX" << std::endl;

  Status_t status;

  // Load MSSD graph
  TargetInfo     lRefInfo  = TargetInfo();
  ApexTargetInfo lApexInfo = ApexTargetInfo();
  auto           lWorkspace =
      std::unique_ptr<Workspace>(new Workspace(std::map<std::string, TargetInfo*>{
          {target::REF, &lRefInfo}, {target::APEX, &lApexInfo}}));

  auto net_mssd = std::unique_ptr<Graph>(new Graph(lWorkspace.get()));

#ifndef _WINDOWS
  Tensor* lApexNetInput = net_mssd->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("APEX_NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
                       TensorShape<TensorFormat_t::NHWC>{1, 300, 300, 3}, 
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

  // Generate default anchor list
  int   numLayers             = 6;
  float minScale              = 0.2;
  float maxScale              = 0.95;
  float aspectRatios[]        = {1.f, 2.f, 1.f / 2, 3.0, 1.f / 3};
  int   numAspectRatios       = 5;
  int   numFeatureMaps        = 6;
  int   featureMapShapeList[] = {
      19, 19, 
      10, 10, 
      5, 5, 
      3, 3, 
      2, 2, 
      1, 1,
  };

  ssd::MultipleGridAnchorGenerator *anchorGenerator;
  ssd::createSsdAnchors(&anchorGenerator, numLayers, minScale, maxScale, numAspectRatios, aspectRatios,
                                    nullptr, true, 1.0f, anchorGenVer);

  // Create default anchor lists anchorList
  BoxList* anchorList;
  ssd::generateAnchors(anchorGenerator, &anchorList, numFeatureMaps, featureMapShapeList);
  cornerToCentreBox(anchorList);

  // Decode boxes
  anchorParams* anchParams = new anchorParams;
  anchParams->y_scale      = 10.0f;
  anchParams->x_scale      = 10.0f;
  anchParams->height_scale = 5.0f;
  anchParams->width_scale  = 5.0f;

#ifndef _WINDOWS
  io_FrameOutput_t lFrameOutput;
  lFrameOutput.Init(DISPLAY_SCENE_WIDTH, DISPLAY_SCENE_HEIGHT, io::IO_DATA_DEPTH_08,
                    io::IO_DATA_CH3);
#endif

  std::ifstream            labelfile(aLabels);
  std::vector<std::string> classLabels;
  std::string              line;

  while(std::getline(labelfile, line))
  {
    classLabels.push_back(line);
  }

  std::vector<std::pair<int, float>> results;

 // cv::Mat* outputImage = new cv::Mat();

  cv::Mat outputImage;





  
  //******************** vedio detect**********************
  
  
	 cv::VideoCapture capture; /* open video */
	  // capture.open("data/common/mvi_0050.avi");
	   capture.open("data/common/mvi_0050.avi");
	 
	   if(!capture.isOpened()) // if not success, exit program
	   {
		 printf("Cannot open the video file: \n");
		//return -1;
	   } // if video open failed
	 
	 outputImage = cv::Mat::zeros(cv::Size(1280, 720), CV_8UC3);




	 double fps;
	 char string[10];  // 用于存放帧率的字符串
	 double t = 0;




	 //cv::VideoWriter writer("FCW.avi", CV_FOURCC('M', 'P', '4', '2'), 25.0, cv::Size(640, 400));




//  while(true)
//  {

  
    std::ifstream infile(aDescriptionFile);
    std::string   imagePath;





	

 
		while(true)
		{	



		t = (double)cv::getTickCount();

		

		   capture >> outputImage;

		
			if (!capture.read(outputImage))
			  {
			  printf("\nread video frame failed!\n"); /* capture opencv frame */
			  //capture.set(1,0);
			  continue;
			  }
	  
	  
			if(!outputImage.data)
			{
			  std::cout << "SSD: could not open or find image" << std::endl;
			  return {};
			}
			


			stopwatch(true);
			resizeBilinearAndNormalize(outputImage, lApexNetInput, true, {128}, 1.0f);
			stopwatch(false, "Resize");
	  
	
 /*

			stopwatch(true);
			ReadImageToTensor(outputImage, lApexNetInput, true, {128}, 1.0);
			ReadImageToTensor(imagePath, lApexNetInput, outputImage, 128, 128, 0)
			stopwatch(false, "ReadImageToTensor");



	  */ 




	   
	  
	  //***********************end of add avi detect
	  
	 

      lApexNetInput->Flush();
      output[0]->Invalidate();
      output[1]->Invalidate();

      // 2. Run preprocessed img through MSSD to produce raw class and box prediction results
      stopwatch(true);
      status = net_mssd->Run();

      stopwatch(false, "mssd object detection");
      if(Status_t::SUCCESS != status)
      {
        std::cout << "APEX net verification failed" << std::endl;
        return -1;
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
      if(nullptr == boxOutputFixedPoint || nullptr == classOutputFixedPoint)
      {
        std::cout << "Net output box/class not existed " << std::endl;
        return -1;
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

     // std::vector<BoundingBox> bboxes = multiclassNonMaxSuppression(boxes, scorePredictorList, 1, 0.35, 0.6, 100, 100, NULL, false, true);



	 stopwatch(true);

      std::vector<BoundingBox> bboxes = multiclassNonMaxSuppression(boxes, scorePredictorList, 1, 0.35, 0.6, 100, 100, NULL, false, true);

	  stopwatch(false, "multiclassNonMaxSuppression");

//********* 		  
     // cv::Mat outImage = *outputImage;



	   cv::Mat outImage=outputImage.clone();

//****




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
        cv::putText(outImage, label, origin, cv::FONT_HERSHEY_SIMPLEX, scale,
                    CV_RGB(255, 255, 255));
        imageResult += std::to_string(ymin) + ", " + std::to_string(xmin) + ", " +
                       std::to_string(ymax) + ", " + std::to_string(xmax) + ", " +
                       std::to_string(a.classNo) + ", " + std::to_string(a.score) + "\n";

        results.push_back(std::pair<int, float>(a.classNo, a.score));

      }
#ifdef _WINDOWS
      cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);// Create a window for display.
      cv::imshow("Display window", outImage);                // Show our image inside it.
      cv::waitKey(2000);

#else



	  t=((double)cv::getTickCount()-t)/cv::getTickFrequency();

	  fps=1.0/t;
	  sprintf(string,"%.2f",fps);
	  std::string fpsString("FPS:");
	  fpsString+=string;
	  putText(outImage,
	  	      fpsString,
	  	      cv::Point(5,20),
	  	      cv::FONT_HERSHEY_COMPLEX,
	  	      0.5,
	  	      cv::Scalar(0,0,0));
	  


//writer<<outImage;


      DisplayImageOD(lFrameOutput, outImage, classLabels, results);

     // sleep(1);
	  
#endif
      std::cout << imageResult << std::endl;
      outImage.release();
      delete[] boxPredictorList->boxList;
      delete[] scorePredictorList->scores;
      delete boxPredictorList;
      delete scorePredictorList;
      results.clear();
    }
 // }
  delete anchParams;
  delete[] anchorList->boxList;
  delete[] anchorList;

}
