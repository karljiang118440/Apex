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



//************* show the video in screen



#ifdef __STANDALONE__
#include "frame_output_dcu.h"
#define CHNL_CNT io::IO_DATA_CH3
#else // #ifdef __STANDALONE__
#include "frame_output_v234fb.h"
#define CHNL_CNT io::IO_DATA_CH3
#endif // else from #ifdef __STANDALONE__




#ifdef __STANDALONE__
  io::FrameOutputDCU output(1280, 720,	io::IO_DATA_DEPTH_08, CHNL_CNT);
#else	
  io::FrameOutputV234Fb output(1280, 720, io::IO_DATA_DEPTH_08, CHNL_CNT);
#endif




//**************************************




// Calls the demo app that takes in the arguments as mentioned below.
int main(int argc, char** argv)
{
  std::string str[8] = {"mobilenet classification loop demo", "mnetv1_ssd object detection loop demo",
                        "mnetv2_ssd object detection loop demo","mnetv2_ssdlite object detection loop demo",
                        "mobilenet classification single image demo", "mnetv1_ssd object detection single image demo",
                        "mnetv2_ssd object detection single image demo","mnetv2_ssdlite object detection single image demo"};
  int         choice, status = 1;
#ifndef _WINDOWS
  APEX_Init();
#endif


// show the video_detect**********


	////////////////////////////////////////////////////////////////////////////
	// DCU out setup
#if video_airunner
  /* 
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
*/



printf("output.PutFrame(lFrameOutput) is finished.\n");

output.PutFrame(lFrameOutput);

#endif

//**************show the video_detect

  if(argc < 2 || argc > 2)
  {
    std::cerr << "Too few or too many arguments" << std::endl;
    std::cout << "usage: " << argv[0] << "(1/2/3/4/5/6/7/8/9)" << std::endl;
    std::cout << "1: mobilenet classification loop demo" << std::endl;
    std::cout << "2: mnetv1_ssd object detection loop demo. " << std::endl;
    std::cout << "3: mnetv2_ssd object detection loop demo. " << std::endl;
    std::cout << "4: mnetv2_ssdlite object detection loop demo. " << std::endl;
    std::cout << "5: mobilenet classification single image demo." << std::endl;
    std::cout << "6: mnetv1_ssd object detection single image demo." << std::endl;
    std::cout << "7: mnetv2_ssd object detection single image demo." << std::endl;
    std::cout << "8: mnetv2_ssdlite object detection single image demo." << std::endl;
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
      status = mobilenet_loop("data/airunner/frozen_mobilenet_q_sym_final_part.pb",
                              "data/airunner/frozen_mobilenet_float_outputlayers_graph.pb",
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
      status = case_mobilenet("data/airunner/frozen_mobilenet_q_sym_final_part.pb",
                              "data/airunner/frozen_mobilenet_float_outputlayers_graph.pb",
                              "data/airunner/test_classification.jpg",
                              "data/airunner/image_classification/imagenet_slim_labels.txt");
      break;



#if 0   // frozen_mssd_v2_part_bn_quant_final.pb 模型
    case 6:
      status =
          case_mssd("data/airunner/frozen_mssd_v2_part_bn_quant_final.pb",  //ssd_mb1_pet_part_bn_quant  <-- frozen_mssd_afterbnfold_iden_part_quant_in
                    "data/airunner/object_detection/cat.jpg", "data/airunner/object_detection/mscoco_labels.txt");  // mscoco_labels --> pet_labels
      break;
#endif


#if 1   // frozen_mssd_v2_part_bn_quant.pb 模型
    case 6:
      status =
          case_mssd("data/airunner/frozen_mssd_v2_part_bn_quant.pb",
                    "data/airunner/object_detection/dog.jpg", "data/airunner/object_detection/mscoco_labels.txt");
      break;

#endif 

# if 0  // raw case 6
    case 6:`
      status =
          case_mssd("data/airunner/frozen_mssd_afterbnfold_iden_part_quant_in.pb",
                    "data/airunner/test_object_detection.jpg", "data/airunner/object_detection/mscoco_labels.txt");
      break;

#endif 

# if 0  // frozen_mssd.pb --> frozen_mssd_part_bn_quant_final.pb
    case 6:
      status =
          case_mssd("data/airunner/frozen_mssd_part_bn_quant_final.pb",
                    "data/airunner/test_object_detection.jpg", "data/airunner/object_detection/mscoco_labels.txt");
      break;

#endif 





    case 7:
      status =

      #if 0 // row model
          case_mssd("data/airunner/mnet2ssd_inference_graph_part_bn_quant_final.pb",
                    "data/airunner/test_object_detection.jpg", "data/airunner/object_detection/mscoco_labels.txt", 91, 2);
      #endif

      #if 1  // frozen_ssd_mb2_quantized_part_bn_quant_final.pb
          case_mssd("data/airunner/frozen_ssd_mb2_quantized_part_bn_quant_final.pb",
                    "data/airunner/test_object_detection.jpg", "data/airunner/object_detection/mscoco_labels.txt", 91, 2);
      #endif


      break;
	  
    case 8:

      status =

      #if 0  // raw model
          case_mssd("data/airunner/frozen_mssd_v2_lite_part_bn_quant_final2.pb",
                   "data/airunner/test_object_detection.jpg", "data/airunner/object_detection/mscoco_labels.txt", 91, 2);
      #endif


      #if 1
          case_mssd("data/airunner/frozen_ssdlite_mb2_part_bn_quant_final.pb", 
                    "data/airunner/object_detection/dog.jpg", "data/airunner/object_detection/mscoco_labels.txt", 91, 2);

      #endif



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
  return 0;
}
