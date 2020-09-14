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



#define DUMP_RESULT 0

using namespace airunner;


// Runs two graphs split using tensorflow
std::vector<std::pair<uint32_t, float>> case_network_target(const std::string& aMnetGraphP1,
                                                            const std::string& aMnetGraphP2,
                                                            const std::string& aImageFile,
                                                            const std::string& aSlimLabelsFile,
                                                            const std::string& aTarget,
                                                            bool  isFixed,
                                                            bool  isExtSoftmax,
                                                            bool  flipDisplay)
{
  (void)flipDisplay;

  std::cout << "Running graph for " << aTarget << std::endl;
  Status_t status;

  // Creating a new workspace
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();

  std::string lCpuString = TargetType::CPU_REF();

  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Create the two subgraphs
  auto net_mobile      = std::unique_ptr<Graph>(new Graph(*lWorkspace));
  auto netFixedToFloat = std::unique_ptr<Graph>(new Graph(*lWorkspace));

  // Configure input to first graph
  Tensor* lApexNetInput;
  if(!isFixed)
  {
    lApexNetInput = net_mobile->AddTensor(std::unique_ptr<Tensor>(
        Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 150, 150, 3},
                        TensorLayout<TensorFormat_t::NHWC>())));
  }
  else
  {
    lApexNetInput = net_mobile->AddTensor(std::unique_ptr<Tensor>(
        Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT, 
                        TensorShape<TensorFormat_t::NHWC>{1, 150, 150, 3},
                        TensorLayout<TensorFormat_t::NHWC>())));
  }

  // Load first graph
  std::vector<Tensor*> fixedOutput;
  status = LoadNetFromTensorFlow(*net_mobile, aMnetGraphP1, lApexNetInput, fixedOutput);
  if(Status_t::SUCCESS != status || fixedOutput.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }

  lApexNetInput->SetQuantParams({QuantInfo(0, 1)});
  lApexNetInput->Allocate(Allocation_t::OAL);

  status = net_mobile->SetTargetHint(TargetType::CPU_REF());
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Set CPU target failed" << std::endl;
    return {};
  }

  status = net_mobile->Prepare();
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Net verification failed" << std::endl;
    return {};
  }

  // Configure input to second graph
  int     dimN        = fixedOutput[0]->Dim(3);
  int     dimH        = fixedOutput[0]->Dim(2);
  int     dimW        = fixedOutput[0]->Dim(1);
  int     dimC        = fixedOutput[0]->Dim(0);
  Tensor* floatInterm = netFixedToFloat->AddTensor(std::unique_ptr<Tensor>(Tensor::Create<>(
      "FLOAT_INTERM_TENSOR",  DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 1, 1, 6272},
      TensorLayout<TensorFormat_t::NHWC>())));
  floatInterm->Allocate(Allocation_t::HEAP);

  // Load second graph
  std::vector<Tensor*> Output;
  status = LoadNetFromTensorFlow(*netFixedToFloat, aMnetGraphP2, floatInterm, Output);
  if(Status_t::SUCCESS != status || Output.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }

  status = netFixedToFloat->SetTargetHint(lCpuString);

  if(Status_t::SUCCESS != status)
  {
    std::cout << "Set target failed" << std::endl;
    return {};
  }

  status = netFixedToFloat->Prepare();
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Net verification failed" << std::endl;
    return {};
  }

  // Load class labels
  std::vector<std::string> classLabels;
  LoadTxtFile(aSlimLabelsFile, classLabels);

  std::cout << "Classifying: " << aImageFile << std::endl;
  cv::Mat image = cv::imread(aImageFile);
  if(!image.data)
  {
    std::cout << "Could not open or find image: " << aImageFile << std::endl;
    return {};
  }
  std::unique_ptr<Tensor> imageInput = std::unique_ptr<Tensor>(Tensor::Create<>(
        "tensor",  DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 150, 150, 3},
        TensorLayout<TensorFormat_t::NHWC>()));
        
  imageInput->Allocate(Allocation_t::HEAP);

  if(Status_t::SUCCESS != resizeBilinearAndNormalizeFloat(image, imageInput.get(), {0.0}, {1.0 / 255.0}))
  {
    std::cout << "Failed to read " << aImageFile << std::endl;
    return {};
  }

  lApexNetInput->CopyDataFrom(*imageInput);

  lApexNetInput->Flush();
  fixedOutput[0]->Invalidate();

  // Run network
  status = net_mobile->Run();
  if(status != Status_t::SUCCESS)
  {
    std::cout << "Net execution failed" << std::endl;
    return {};
  }

  // Copy output of first graph to second graph (fixed to float conversion)
  fixedOutput[0]->Invalidate();

  if(isFixed)
  {
    std::unique_ptr<Tensor> intermediateTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
        "tensor",  DataType_t::SIGNED_8BIT, TensorShape<TensorFormat_t::NHWC>{1, 1, 1, 6272},
        TensorLayout<TensorFormat_t::NHWC>()));
        
    intermediateTensor->SetQuantParams(fixedOutput[0]->QuantParams());
    intermediateTensor->Allocate(Allocation_t::HEAP);
    intermediateTensor->CopyDataFrom(*fixedOutput[0]);
    floatInterm->CopyDataFrom(*intermediateTensor);
  }
  else
  {
    floatInterm->CopyDataFrom(*fixedOutput[0]);
  }

  status = netFixedToFloat->Run();
  if(status != Status_t::SUCCESS)
  {
    std::cout << "Fixed to float net execution failed" << std::endl;
    return {};
  }

  float result = *(Output[0]->DataPtr<float>());
  float sigmoid = 1 / (1 + exp(-result));

  std::cout << "Dog: " << sigmoid << " Cat: " << 1 - sigmoid << std::endl;

   return {};
}

// Run graph using splitting tools
std::vector<std::pair<uint32_t, float>> case_network_target_split(const std::string& aMnetGraph1,
                                                                  const std::string& aMnetGraph2,
                                                                  const std::string& aImageFile,
                                                                  const std::string& aSlimLabelsFile,
                                                                  const std::string& aTarget,
                                                                  bool  isFixed,
                                                                  bool  isExtSoftmax,
                                                                  bool  flipDisplay)
{
  (void)flipDisplay;

  std::cout << "Running graph for " << aTarget << std::endl;
  Status_t status;

  // Creating a new workspace
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();

  std::string lCpuString = TargetType::CPU_REF();

  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Create the two subgraphs
  auto net_mobile      = std::unique_ptr<Graph>(new Graph(*lWorkspace));
  auto netFixedToFloat = std::unique_ptr<Graph>(new Graph(*lWorkspace));

  // Configure input to first graph
  Tensor* lApexNetInput;
  if(!isFixed)
  {
    lApexNetInput = net_mobile->AddTensor(std::unique_ptr<Tensor>(
        Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 150, 150, 3},
                        TensorLayout<TensorFormat_t::NHWC>())));
  }
  else
  {
    lApexNetInput = net_mobile->AddTensor(std::unique_ptr<Tensor>(
        Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT, 
                        TensorShape<TensorFormat_t::NHWC>{1, 150, 150, 3},
                        TensorLayout<TensorFormat_t::NHWC>())));
  }

  // Load first graph
  std::string lOutputTensor = "max_pooling2d_4/MaxPool";
  std::map<std::string, Tensor*> lInputTensors = {{"conv2d_1_input", lApexNetInput}};
  std::map<std::string, Tensor*> lOutputTensors;
  lOutputTensors[lOutputTensor];

  status = LoadNetFromTensorFlowV2(*net_mobile, aMnetGraph1, lInputTensors, lOutputTensors, "airsplit1.log");
  if(Status_t::SUCCESS != status || lOutputTensors.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }
  
  lApexNetInput->SetQuantParams({QuantInfo(0, 1)});
  lApexNetInput->Allocate(Allocation_t::OAL);

  status = net_mobile->SetTargetHint(TargetType::CPU_REF());
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Set CPU target failed" << std::endl;
    return {};
  }

  status = net_mobile->Prepare();
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Net verification failed" << std::endl;
    return {};
  }

  // Configure input to second graph
  int     dimN        = lOutputTensors[lOutputTensor]->Dim(3);
  int     dimH        = lOutputTensors[lOutputTensor]->Dim(2);
  int     dimW        = lOutputTensors[lOutputTensor]->Dim(1);
  int     dimC        = lOutputTensors[lOutputTensor]->Dim(0);
  Tensor* floatInterm = netFixedToFloat->AddTensor(std::unique_ptr<Tensor>(Tensor::Create<>(
      "FLOAT_INTERM_TENSOR",  DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 1, 1, 6272},
      TensorLayout<TensorFormat_t::NHWC>())));
  floatInterm->Allocate(Allocation_t::HEAP);

  // Load second graph
  std::string lOutputTensor1 = "dense_2/BiasAdd";
  std::map<std::string, Tensor*> lInputTensors1 = {{"dropout_1/Identity", floatInterm}};
  std::map<std::string, Tensor*> lOutputTensors1;
  lOutputTensors1[lOutputTensor1];

  status = LoadNetFromTensorFlowV2(*netFixedToFloat, aMnetGraph2, lInputTensors1, lOutputTensors1, "airsplit2.log");
  if(Status_t::SUCCESS != status || lOutputTensors1.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }

  status = netFixedToFloat->SetTargetHint(lCpuString);

  if(Status_t::SUCCESS != status)
  {
    std::cout << "Set target failed" << std::endl;
    return {};
  }

  status = netFixedToFloat->Prepare();
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Net verification failed" << std::endl;
    return {};
  }

  // Load class labels
  std::vector<std::string> classLabels;
  LoadTxtFile(aSlimLabelsFile, classLabels);

  std::cout << "Classifying: " << aImageFile << std::endl;
  cv::Mat image = cv::imread(aImageFile);
  if(!image.data)
  {
    std::cout << "Could not open or find image: " << aImageFile << std::endl;
    return {};
  }
  std::unique_ptr<Tensor> imageInput = std::unique_ptr<Tensor>(Tensor::Create<>(
        "tensor",  DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 150, 150, 3},
        TensorLayout<TensorFormat_t::NHWC>()));
        
  imageInput->Allocate(Allocation_t::HEAP);

  if(Status_t::SUCCESS != resizeBilinearAndNormalizeFloat(image, imageInput.get(), {0.0}, {1.0 / 255.0}))
  {
    std::cout << "Failed to read " << aImageFile << std::endl;
    return {};
  }

  lApexNetInput->CopyDataFrom(*imageInput);

  lApexNetInput->Flush();
  lOutputTensors[lOutputTensor]->Invalidate();

  // Run network
  status = net_mobile->Run();
  if(status != Status_t::SUCCESS)
  {
    std::cout << "Net execution failed" << std::endl;
    return {};
  }

  // Copy output of first graph to second graph (fixed to float conversion)
  lOutputTensors[lOutputTensor]->Invalidate();

  if(isFixed)
  {
    std::unique_ptr<Tensor> intermediateTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
        "tensor",  DataType_t::SIGNED_8BIT, TensorShape<TensorFormat_t::NHWC>{1, 1, 1, 6272},
        TensorLayout<TensorFormat_t::NHWC>()));
        
    intermediateTensor->SetQuantParams(lOutputTensors[lOutputTensor]->QuantParams());
    intermediateTensor->Allocate(Allocation_t::HEAP);
    intermediateTensor->CopyDataFrom(*lOutputTensors[lOutputTensor]);
    floatInterm->CopyDataFrom(*intermediateTensor);
  }
  else
  {
    status = floatInterm->CopyDataFrom(*lOutputTensors[lOutputTensor]);
    if(status != Status_t::SUCCESS)
    {
      std::cout << "Copy failed" << (int)status << std::endl;
      return {};
    }
  }

  status = netFixedToFloat->Run();
  if(status != Status_t::SUCCESS)
  {
    std::cout << "Fixed to float net execution failed" << std::endl;
    return {};
  }

  float result = *(lOutputTensors1[lOutputTensor1]->DataPtr<float>());
  float sigmoid = 1 / (1 + exp(-result));

  std::cout << "Dog: " << sigmoid << " Cat: " << 1 - sigmoid << std::endl;

  return {};
}

std::vector<std::pair<uint32_t, float>> case_network_target_full(const std::string& aMnetGraph,
                                                              const std::string& aImageFile,
                                                              const std::string& aSlimLabelsFile,
                                                              const std::string& aTarget,
                                                              bool  isFixed,
                                                              bool  isExtSoftmax,
                                                              bool  flipDisplay)
{
  (void)flipDisplay;

  std::cout << "Running Network graph for " << aTarget << std::endl;
  Status_t status;

  // Creating a new workspace
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();

  std::string lCpuString = TargetType::CPU_REF();

  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Create the two subgraphs
  auto net_mobile      = std::unique_ptr<Graph>(new Graph(*lWorkspace));
  auto netFixedToFloat = std::unique_ptr<Graph>(new Graph(*lWorkspace));

  // Configure input to first graph
  Tensor* lApexNetInput;
  if(!isFixed)
  {
    lApexNetInput = net_mobile->AddTensor(std::unique_ptr<Tensor>(
        Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 32,32, 3},
                        TensorLayout<TensorFormat_t::NHWC>())));
  }
  else
  {
    lApexNetInput = net_mobile->AddTensor(std::unique_ptr<Tensor>(
        Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT, 
                        TensorShape<TensorFormat_t::NHWC>{1, 32,32, 3},
                        TensorLayout<TensorFormat_t::NHWC>())));
  }

  // Load first graph
  //std::string lOutputTensor = "CifarNet/logits/BiasAdd";
  std::string lOutputTensor = "CifarNet/Predictions/Softmax";
  std::map<std::string, Tensor*> lInputTensors = {{"input", lApexNetInput}};
  std::map<std::string, Tensor*> lOutputTensors;
  lOutputTensors[lOutputTensor];

  status = LoadNetFromTensorFlowV2(*net_mobile, aMnetGraph, lInputTensors, lOutputTensors, "cifarnet.log");
  if(Status_t::SUCCESS != status || lOutputTensors.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }

  lOutputTensors[lOutputTensor]->SetQuantParams({QuantInfo(0, 1)});
  lOutputTensors[lOutputTensor]->Allocate(Allocation_t::OAL);

  status = net_mobile->SetTargetHint(TargetType::CPU_REF());
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Set CPU target failed" << std::endl;
    return {};
  }

  status = net_mobile->Prepare();
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Net verification failed" << std::endl;
    return {};
  }

  std::cout << "Classifying: " << aImageFile << std::endl;
  cv::Mat image = cv::imread(aImageFile);
  if(!image.data)
  {
    std::cout << "Could not open or find image: " << aImageFile << std::endl;
    return {};
  }
  std::unique_ptr<Tensor> imageInput = std::unique_ptr<Tensor>(Tensor::Create<>(
        "tensor",  DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 32,32, 3},
        TensorLayout<TensorFormat_t::NHWC>()));
        
  imageInput->Allocate(Allocation_t::HEAP);

  if(Status_t::SUCCESS != resizeBilinearAndNormalizeFloat(image, imageInput.get(), {0.0}, {1.0 / 255.0}))
  {
    std::cout << "Failed to read " << aImageFile << std::endl;
    return {};
  }

  lApexNetInput->CopyDataFrom(*imageInput);

  lApexNetInput->Flush();
  lOutputTensors[lOutputTensor]->Invalidate();

  // Run network
  status = net_mobile->Run();
  if(status != Status_t::SUCCESS)
  {
    std::cout << "Net execution failed" << std::endl;
    return {};
  }

  // Copy output of first graph to second graph (fixed to float conversion)
  lOutputTensors[lOutputTensor]->Invalidate();


#if 1 // 结果暂不处理，仅仅是提取到目标参数即可

  float result = *(lOutputTensors[lOutputTensor]->DataPtr<float>());
  float sigmoid = static_cast<float32_t>(1. / (1. + exp((result) * -1.)));

  std::cout << "Dog: " << sigmoid << " Cat: " << 1 - sigmoid << std::endl;

#endif

   return {};
}


int case_mobilenet_pre_split(const std::string& aMnetGraphP1,
                             const std::string& aMnetGraphP2,
                             const std::string& aImageFile,
                             const std::string& aSlimLabelsFile,
                             bool  isFixed,
                             bool  isExtSoftmax,
                             bool  flipDisplay)
{
  if (!isFixed)
  {
    auto results_cpu  = case_network_target(aMnetGraphP1, aMnetGraphP2, aImageFile, aSlimLabelsFile, "CPU",  isFixed, isExtSoftmax, flipDisplay);
  }
  else
  {
    auto results_cpu  = case_network_target(aMnetGraphP1, aMnetGraphP2, aImageFile, aSlimLabelsFile, "CPU",  isFixed, isExtSoftmax, flipDisplay);
  }

  return 0;
}

int case_mobilenet_internal_split(const std::string& aMnetGraph1,
                        const std::string& aMnetGraph2,
                        const std::string& aImageFile,
                        const std::string& aSlimLabelsFile,
                        bool  isFixed,
                        bool  isExtSoftmax,
                        bool  flipDisplay)
{
  if (!isFixed)
  {
    auto results_split = case_network_target_split(aMnetGraph1, aMnetGraph2, aImageFile, aSlimLabelsFile, "CPU",  isFixed, isExtSoftmax, flipDisplay);
  }
  else
  {
    auto results_split  = case_network_target_split(aMnetGraph1, aMnetGraph2, aImageFile, aSlimLabelsFile, "CPU",  isFixed, isExtSoftmax, flipDisplay);
  }

  return 0;
}


int case_mobilenet_full(const std::string& aMnetGraph,
                        const std::string& aImageFile,
                        const std::string& aSlimLabelsFile,
                        bool  isFixed,
                        bool  isExtSoftmax,
                        bool  flipDisplay)
{
  auto results_full  = case_network_target_full(aMnetGraph, aImageFile, aSlimLabelsFile, "APEX",  isFixed, isExtSoftmax, flipDisplay);
  //auto results_full  = case_network_target_full(aMnetGraph, aImageFile, aSlimLabelsFile, "CPU",  isFixed, isExtSoftmax, flipDisplay);
  return 0;
}
