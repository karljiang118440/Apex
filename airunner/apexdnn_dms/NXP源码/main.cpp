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

#ifndef _WINDOWS
#include "apex.h"
#endif

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "read_image.hpp"
#include "ai_app_task.hpp"


// Input resolution
const int  RESX        = 1280;
const int  RESY        = 720;

#ifdef cam_airunner
ISPCameraInputStereo ai_camerainput;
io::FrameOutputV234Fb lDcuOutput;
#endif


// Calls the demo app that takes in the arguments as mentioned below.
int main(int argc, char** argv)
{
  std::string str[] = {"mobilenet classification loop demo", "mnetv1_ssd object detection loop demo",
                        "mnetv2_ssd object detection loop demo","mnetv2_ssdlite object detection loop demo",
                        "mobilenet classification single image demo", "mnetv1_ssd object detection single image demo",
                        "mnetv2_ssd object detection single image demo","mnetv2_ssdlite object detection single image demo"};
  int         choice, status = 1;
#ifndef _WINDOWS
  APEX_Init();
#endif


  ////////////////////////////////////////////////////////////////////////////
  // DCU out setup
#if 1

#ifdef __STANDALONE__
	io::FrameOutputDCU lDcuOutput(WIDTH, HEIGHT, io::IO_DATA_DEPTH_08, io::IO_DATA_CH3);
#else
	// setup Ctrl+C handler
	if(ISPCameraInputStereo::SigintSetup() != SEQ_LIB_SUCCESS)
	{
	  VDB_LOG_ERROR("Failed to register Ctrl+C signal handler.");
	  return -1;
	}
  
	printf("Press Ctrl+C to terminate the demo.\n");
  
	lDcuOutput.Init(RESX, RESY, io::IO_DATA_DEPTH_08, io::IO_DATA_CH3);
#endif

#endif

  ////////////////////////////////////////////////////////////////////////////
  // ISP in setup
#ifdef cam_airunner
  // setup Ctrl+C handler
   if(ISPCameraInputStereo::SigintSetup() != SEQ_LIB_SUCCESS)
   {
	 VDB_LOG_ERROR("Failed to register Ctrl+C signal handler.");
	 return -1;
   }

  ai_camerainput.Init();
  ai_camerainput.Start();
#endif  
  ////////////////////////////////////////////////////////////////////////////
#if 0

  LIB_RESULT libResult = LIB_FAILURE;
  
  AIRunner_App app;
  
  libResult = app.Start();
  SHOW_ERROR(libResult == LIB_SUCCESS);
  return 0;
#endif	
///////////////////////////////////////////
   


  if(argc < 2 || argc > 2)
  {
    std::cerr << "Too few or too many arguments" << std::endl;
    std::cout << "usage: " << argv[0] << "(1/2/3/4/5/6/7/8/9)" << std::endl;
    std::cout << "1: " << str[0] << std::endl;
    std::cout << "2: " << str[1] << std::endl;
    std::cout << "3: " << str[2] << std::endl;
    std::cout << "4: " << str[3] << std::endl;
    std::cout << "5: " << str[4] << std::endl;
    std::cout << "6: " << str[5] << std::endl;
    std::cout << "7: " << str[6] << std::endl;
    std::cout << "8: " << str[7] << std::endl;
    std::cout << "9: Exit the program. " << std::endl << std::endl;

    std::cout << "Enter test index. ";
    std::cin >> choice;
  }
  else
  {
    choice = atoi(argv[1]);
  }

  switch(choice)
  {
    case 1:
      status = mobilenet_loop("data/airunner/frozen_mobilenet_bn_qsym_final.pb",
                              "data/airunner/frozen_mobilenet_float_outputlayers_graph.pb",/*"data/airunner/frozen_mobilenet_float_outputlayers_graph.pb",*/
                              "data/airunner/image_classification/description.txt",
                              "data/airunner/image_classification/imagenet_slim_labels.txt");
      break;
    case 2:
      status = mssd_apex_loop("data/airunner/frozen_mssd_afterbnfold_iden_part_quant_in.pb",
                              "data/airunner/object_detection/description.txt",
                              "data/airunner/object_detection/mscoco_labels.txt");
      break;
    case 3:
      status = mssd_apex_loop("data/airunner/mnet2ssd_inference_graph_part_bn_quant_final.pb",
                              "data/airunner/object_detection/description.txt",
                              "data/airunner/object_detection/mscoco_labels.txt", 91, 2);
      break;
    case 4:
      status = mssd_apex_loop("data/airunner/frozen_mssd_v2_lite_part_bn_quant_final2.pb",
                              "data/airunner/object_detection/description.txt",
                              "data/airunner/object_detection/mscoco_labels.txt", 91, 2);
      break;
    case 5:
      status = case_mobilenet(/*"data/airunner/frozen_mobilenet_q_sym_final_part.pb",*/"data/airunner/mymodel_part_bn_quant_final-intesight.pb",
                              "data/airunner/frozen_mobilenet_float_outputlayers_graph.pb",/*"data/airunner/frozen_mobilenet_float_outputlayers_graph.pb",*/
                              "data/airunner/test_classification.jpg",
                              "data/airunner/image_classification/imagenet_slim_labels.txt");
      break;
    case 6:
      status =
          case_mssd("data/airunner/frozen_mssd_afterbnfold_iden_part_quant_in.pb",
                    "data/airunner/test_object_detection.jpg", "data/airunner/object_detection/mscoco_labels.txt");
      break;
    case 7:
      status =
          case_mssd("data/airunner/mnet2ssd_inference_graph_part_bn_quant_final.pb",
                    "data/airunner/test_object_detection.jpg", "data/airunner/object_detection/mscoco_labels.txt", 91, 2);
      break;
    case 8:
      status =
          case_mssd("data/airunner/frozen_mssd_v2_lite_part_bn_quant_final2.pb",
                    "data/airunner/test_object_detection.jpg", "data/airunner/object_detection/mscoco_labels.txt", 91, 2);
      break;
    case 9:
    {
      std::cout << "Exiting .... " << std::endl;
      return 0;
    }
    default:
      std::cout << "This option is not supported .... " << std::endl;
      return 0;
  }

  if(choice < 9)
  {
    if(status == 0)
      std::cout << str[choice - 1] << " Finished successfully." << std::endl << std::endl;
    else
      std::cout << str[choice - 1] << " Failed to finish." << std::endl << std::endl;
  }

  #ifdef cam_airunner
  //*** Stop ISP processing ***
	ai_camerainput.Stop();
	ai_camerainput.Deinit();
  #endif	

  return 0;
}
