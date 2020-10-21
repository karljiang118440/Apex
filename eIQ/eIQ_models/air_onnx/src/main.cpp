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

#ifdef __aarch64__
  #include "apex.h"
#endif

using namespace airunner;

#ifndef USE_ONLY_PYTORCH_VERSION
  // #ifndef ENABLE_SAVE_SUBGRAPH
  // #define ENABLE_SAVE_SUBGRAPH
  // #endif // !ENABLE_SAVE_SUBGRAPH
#endif // !USE_ONLY_PYTORCH_VERSION



#ifdef ENABLE_SAVE_SUBGRAPH
  constexpr auto SUBGRAPH_OUT_PATH = "data/airunner/";
#endif // ENABLE_SAVE_SUBGRAPH

// Calls the demo app that takes in the arguments as mentioned below.
int main(int argc, char **argv)
{
  std::string str[] = { "ResNet18 image classification floating point demo, pytorch version",
                        "ResNet18 image classification floating point demo, split pytorch version",
                        "ResNet18 image classification fixed point demo, split pytorch version",
#ifndef USE_ONLY_PYTORCH_VERSION
                        "ResNet18-v1 image classification floating point demo, onnx model zoo version",
                        "ResNet18-v1 image classification floating point demo, split onnx model zoo version",
                        "ResNet18-v1 image classification fixed point demo, split onnx model zoo version",
#ifdef ENABLE_SAVE_SUBGRAPH
                        "Splitting ResNet18 model from pytorch",
                        "Splitting ResNet18-v1 model from onnx zoo"
#endif // ENABLE_SAVE_SUBGRAPH
#endif // !USE_ONLY_PYTORCH_VERSION
                        };

  int choice, status = 1;
  bool flip = false;
#ifdef __aarch64__
  APEX_Init();
#endif
  if ((argc != 2) && (argc != 3))
  {
    std::cout << std::endl << "Too few or too many arguments" << std::endl;
    std::cout << std::endl << "Usage: " << argv[0] << "(1/2/3/4/5/6) [-f]" << std::endl;
    std::cout << "1: " << str[0] << std::endl;
    std::cout << "2: " << str[1] << std::endl;
    std::cout << "3: " << str[2] << std::endl;
#ifndef USE_ONLY_PYTORCH_VERSION
    std::cout << "4: " << str[3] << std::endl;
    std::cout << "5: " << str[4] << std::endl;
    std::cout << "6: " << str[5] << std::endl;
#ifdef ENABLE_SAVE_SUBGRAPH
    std::cout << "7: " << str[6] << std::endl;
    std::cout << "8: " << str[7] << std::endl;
#endif // ENABLE_SAVE_SUBGRAPH
#endif // !USE_ONLY_PYTORCH_VERSION
    std::cout << "-f: " << " flip display for lcd" << std::endl;
    std::cout << std::endl << "Enter test index ..." << std::endl;
    std::cin >> choice;
    bool fail = true;
    while(fail)
    {
      std::cout << std::endl << "Flip display? (y/n) ";
      std::string response;
      std::cin >> response;
      if(response == std::string("y"))
      {
        flip = true;
        fail = false;
      }
      else if(response == std::string("n"))
      {
        flip = false;
        fail = false;
      }
      std::cout << std::endl;
    }
  }
  else
  {
    choice = atoi(argv[1]);
    if (argc == 3)
    {
      std::string response = argv[2];
      if(response == std::string("-f"))
      {
        flip = true;
      }
    }
  }

  std::string testImage = "data/airunner/test_classification.jpg";
  std::string testLabels = "data/airunner/image_classification/imagenet_slim_labels_nodummy.txt";

  std::string lRnetGraphFile      = "data/airunner/model_karl/onnx_cifar/LeNet-sim.onnx"; 

  std::string lRnetQuantGraphFile = "data/airunner/test/lenet_pytorch_annotate_minmax.onnx";
#ifndef USE_ONLY_PYTORCH_VERSION
#ifdef ENABLE_SAVE_SUBGRAPH
  std::vector<std::string> lFirstSubGraphInputs   = {"data"};
  std::vector<std::string> lFirstSubGraphOutpus   = {"189"};
  std::vector<std::string> lSecondSubGraphInputs  = {"189"};
  std::vector<std::string> lSecondSubGraphOutpus  = {"output"};
#endif // ENABLE_SAVE_SUBGRAPH
  if (choice > 3)
  {
    if (choice != 7)
    {
      lRnetGraphFile      = "data/airunner/lenet_onnx_zoo_spatial1.onnx";
      lRnetQuantGraphFile = "data/airunner/lenet_onnx_zoo_spatial1_quant.onnx";
    }
#ifdef ENABLE_SAVE_SUBGRAPH
    if (choice == 8)
    {
      lFirstSubGraphInputs  = {"data"};
      lFirstSubGraphOutpus  = {"resnetv15_pool1_fwd"};
      lSecondSubGraphInputs = {"resnetv15_pool1_fwd"};
      lSecondSubGraphOutpus = {"resnetv15_dense0_fwd"};
    }
#endif // ENABLE_SAVE_SUBGRAPH
  }
#endif // !USE_ONLY_PYTORCH_VERSION


double time  = (double)cv::getTickCount();


  switch (choice)
  {
  case 1:
    status = case_resnet( lRnetGraphFile,
                          "",
                          testImage,
                          testLabels, true, true, flip, 1 );
    break;
  case 2:
    status = case_resnet( lRnetGraphFile,
                          "",
                          testImage,
                          testLabels, true, true, flip, 2 );
    break;
  case 3:
    status = case_resnet( lRnetQuantGraphFile,
                          lRnetGraphFile,
                          testImage,
                          testLabels, true, true, flip, 3 );
    break;
#ifndef USE_ONLY_PYTORCH_VERSION
  case 4:
    status = case_resnet( lRnetGraphFile,
                          "",
                          testImage,
                          testLabels, true, true, flip, 4 );
    break;
  case 5:
    status = case_resnet( lRnetGraphFile,
                          "",
                          testImage,
                          testLabels, true, true, flip, 5 );
    break;
  case 6:
    status = case_resnet( lRnetQuantGraphFile,
                          lRnetGraphFile,
                          testImage,
                          testLabels, true, true, flip, 6 );
    break;
#ifdef ENABLE_SAVE_SUBGRAPH
  case 7:
  case 8:
    {
      status = 0;
      std::cout << std::endl << "Network splitting demo running ..." << std::endl;
      std::string lFileName     = getFileName(lRnetGraphFile);
      std::string lOutlSubGraph = SUBGRAPH_OUT_PATH + lFileName + "_part.onnx";
      Status_t lStatus = LoadSubGraphFromONNX(lRnetGraphFile, lFirstSubGraphInputs, lFirstSubGraphOutpus, lOutlSubGraph);
      if(Status_t::SUCCESS != lStatus)
      {
        status = 1;
        break;
      }
      lOutlSubGraph = SUBGRAPH_OUT_PATH + lFileName + "_float_part.onnx";
      lStatus = LoadSubGraphFromONNX(lRnetGraphFile, lSecondSubGraphInputs, lSecondSubGraphOutpus, lOutlSubGraph);
      if(Status_t::SUCCESS != lStatus)
      {
        status = 1;
        break;
      }
      lFileName     = getFileName(lRnetQuantGraphFile);
      lOutlSubGraph = SUBGRAPH_OUT_PATH + lFileName + "_part.onnx";
      lStatus = LoadSubGraphFromONNX(lRnetQuantGraphFile, lFirstSubGraphInputs, lFirstSubGraphOutpus, lOutlSubGraph);
      if(Status_t::SUCCESS != lStatus)
      {
        status = 1;
      }
    }
    break;
#endif // ENABLE_SAVE_SUBGRAPH
#endif // !USE_ONLY_PYTORCH_VERSION
  default:
    std::cout << "This option is not supported ..." << std::endl << std::endl;
    break;
  }


time = (double)cv::getTickCount()- time;
printf("time = %g ms\n",time / (cv::getTickFrequency() * 1000));


#ifdef USE_ONLY_PYTORCH_VERSION
  if ((choice > 0) && (choice < 4))
#else
#ifdef ENABLE_SAVE_SUBGRAPH
  if ((choice > 0) && (choice < 9))
#else
  if ((choice > 0) && (choice < 7))
#endif // ENABLE_SAVE_SUBGRAPH
#endif // USE_ONLY_PYTORCH_VERSION
  {
    if (0 == status)
    {
      std::cout << std::endl << str[choice - 1] << " successfully finished." << std::endl << std::endl;
    }
    else
    {
      std::cout << std::endl << str[choice - 1] << " failed to finish!!!" << std::endl << std::endl;
    }
  }

#ifdef __aarch64__
  APEX_Deinit();
#endif

  return 0;
}
