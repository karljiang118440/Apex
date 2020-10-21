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
#include <airunner_tensor_range.hpp>

using namespace airunner;

void display_image(cv::Mat image, const std::vector<std::string> & labels,
                   const std::vector<std::pair<uint32_t, float>> &results,
                   bool flipDisplay)
{
#ifdef __aarch64__
  io_FrameOutput_t lFrameOutput;
  lFrameOutput.Init(DISPLAY_SCENE_WIDTH, DISPLAY_SCENE_HEIGHT, io::IO_DATA_DEPTH_08, io::IO_DATA_CH3);
  DisplayImageClass(lFrameOutput, image, labels, results, flipDisplay);
  sleep(2);
#endif
}

std::vector<std::pair<uint32_t, float>> case_resnet_target( const std::string& aRnetGraphFile,
                                                            const std::string& aGraphInput,
                                                            const std::string& aImageFile,
                                                            const std::string& aSlimLabelsFile,
                                                            const std::string& aTarget,
                                                            bool  isCPUOpt,
                                                            bool  isExtSoftmax,
                                                            bool  flipDisplay )
{
  if ("APEX" == aTarget)
  {
    std::cout << std::endl << "APEX target not allowed!!!" << std::endl;
    return {};
  }
  
  std::cout << std::endl << "Running \'" << aRnetGraphFile << "\' graph for target " << aTarget << std::endl;

  // Creating a new workspace
  std::string lCpuString = isCPUOpt ? TargetType::CPU_OPT() : TargetType::CPU_REF();
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();
  if (isCPUOpt)
  {
    lTargets[TargetType::CPU_OPT()] = CreateCpuOptTarget();
  }

  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Create the graph
  auto net_Res = std::unique_ptr<Graph>(new Graph(*lWorkspace));
  
  // Configure input to the graph
  Tensor* lInputTensor = net_Res->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 32+6, 32+6, 3},
                       TensorLayout<TensorFormat_t::NHWC>())));

  lInputTensor->Allocate(Allocation_t::HEAP);

  Tensor lInputTensorChild("input.1");
  lInputTensorChild.Configure<>(DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 32, 32, 3});
  lInputTensorChild.SetParent(lInputTensor, {0, 3, 3, 0});

  // Load graph from protobuf model
  std::map<std::string, Tensor*> lInputTensorsMap = {{aGraphInput, lInputTensor}};
  std::vector<Tensor*> lOutputTensorNames;
  
  Status_t status = LoadGraphFromONNX(*net_Res, aRnetGraphFile, lInputTensorsMap, lOutputTensorNames);
  if ((Status_t::SUCCESS != status) || lOutputTensorNames.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }
 
  status = net_Res->SetTargetHint(lCpuString);
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Set CPU target failed" << std::endl;
    return {};
  }

  status = net_Res->Prepare();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net verification failed" << std::endl;
    return {};
  }

  // Load class labels
  std::vector<std::string> classLabels;
  LoadTxtFile(aSlimLabelsFile, classLabels);
  
  std::cout << "\nClassifying \'" << aImageFile << "\' ..." << std::endl;
  cv::Mat image = cv::imread(aImageFile);
  if (!image.data)
  {
    std::cout << "Could not open or find image: " << aImageFile << std::endl;
    return {};
  }

  if (Status_t::SUCCESS != resizeBilinearAndNormalizeFloat(image, &lInputTensorChild, {123.675, 116.28, 103.53}, {0.01712475383, 0.01750700280, 0.01742919389}))
  {
    std::cout << "Failed to read " << aImageFile << std::endl;
    return {};
  }
  lInputTensor->Flush();

  status = net_Res->Run();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net execution failed" << std::endl;
    return {};
  }

  // Apply softmax on output tensor
  std::vector<float> result;
  lOutputTensorNames[0]->Invalidate();
  if (true == isExtSoftmax)
  {
    SoftMax(*lOutputTensorNames[0], result);
  }
  else
  {
    TensorToFloatList(*lOutputTensorNames[0], result);
  }

  // Get top 5 result
  std::vector<std::pair<uint32_t, float>> results;
  processClassificationResults(result, results);

  std::cout << std::endl << "Top 5: " << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[0].first] << ", " << std::to_string(results[0].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[1].first] << ", " << std::to_string(results[1].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[2].first] << ", " << std::to_string(results[2].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[3].first] << ", " << std::to_string(results[3].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[4].first] << ", " << std::to_string(results[4].second)
            << std::endl;
  std::cout << std::endl;

  display_image(image, classLabels, results, flipDisplay);

  return results;
}

Tensor* case_resnet_target_split( const std::string& aRnetGraphFile,
                                  const std::string& aGraphInput,
                                  const std::string& aFirstGraphOutput,
                                  const std::string& aFirstGraphOutNode,
                                  const std::string& aSecondGraphOutput,
                                  const std::string& aImageFile,
                                  const std::string& aSlimLabelsFile,
                                  const std::string& aTarget,
                                  bool  isCPUOpt,
                                  bool  isExtSoftmax,
                                  bool flipDisplay)
{
  if ("APEX" == aTarget)
  {
    std::cout << std::endl << "APEX target not allowed!!!" << std::endl;
    return {};
  }
  std::cout << std::endl << "Running \'" << aRnetGraphFile << "\' graph for target " << aTarget << std::endl;

  // Creating a new workspace
  std::string lCpuString = isCPUOpt ? TargetType::CPU_OPT() : TargetType::CPU_REF();
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();
  if (isCPUOpt)
  {
    lTargets[TargetType::CPU_OPT()] = CreateCpuOptTarget();
  }

  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Create the two subgraphs
  auto net_Res1 = std::unique_ptr<Graph>(new Graph(*lWorkspace));
  auto net_Res2 = std::unique_ptr<Graph>(new Graph(*lWorkspace));

  // Configure input to first graph
  Tensor* lInputTensor1 = net_Res1->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 32+6, 32+6, 3},
                       TensorLayout<TensorFormat_t::NHWC>())));

  lInputTensor1->Allocate(Allocation_t::HEAP);

  Tensor lInputTensorChild1("input");
  lInputTensorChild1.Configure<>(DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 32, 32, 3});
  lInputTensorChild1.SetParent(lInputTensor1, {0, 3, 3, 0});
  
  // Load graph from protobuf model
  std::map<std::string, Tensor*> lInputTensorsMap1 = {{aGraphInput, lInputTensor1}};
  std::map<std::string, Tensor*> lOutputTensorNamesMap1;
  lOutputTensorNamesMap1[aFirstGraphOutput];
 
  Status_t status = LoadGraphFromONNXV2(*net_Res1, aRnetGraphFile, lInputTensorsMap1, lOutputTensorNamesMap1);

  if ((Status_t::SUCCESS != status) || lOutputTensorNamesMap1.empty())
  {
    std::cout << "Failed to load net1" << std::endl;
    return {};
  }

  status = net_Res1->SetTargetHint(lCpuString);
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Set CPU target failed" << std::endl;
    return {};
  }

  Tensor* lNetProbe = net_Res1->GetTensor(aFirstGraphOutNode);
  if (nullptr == lNetProbe)
  {
    std::cout << "tensor not found " << aFirstGraphOutNode << std::endl;
    return {};
  }

  // Ensure the graph tensor isn't allocated from the arena
  net_Res1->SetOutput(lNetProbe);

  status = net_Res1->Prepare();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net1 verification failed" << std::endl;
    return {};
  }
  
  const int32_t lOutCh  = lOutputTensorNamesMap1[aFirstGraphOutput]->Dim(TensorDim_t::OUTPUT_CHANNELS);
  const int32_t lBatch  = lOutputTensorNamesMap1[aFirstGraphOutput]->Dim(TensorDim_t::BATCH);
  const int32_t lHeight = lOutputTensorNamesMap1[aFirstGraphOutput]->Dim(TensorDim_t::HEIGHT);
  const int32_t lWidth  = lOutputTensorNamesMap1[aFirstGraphOutput]->Dim(TensorDim_t::WIDTH);

  Tensor* lInputTensor2 = net_Res2->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{lBatch, lHeight, lWidth, lOutCh},
                       TensorLayout<TensorFormat_t::NHWC>())));

  lInputTensor2->Allocate(Allocation_t::HEAP);

  std::map<std::string, Tensor*> lInputTensorsMap2 = {{aFirstGraphOutput, lInputTensor2}};

  std::map<std::string, Tensor*> lOutputTensorNamesMap2;
  lOutputTensorNamesMap2[aSecondGraphOutput];

  status = LoadGraphFromONNXV2(*net_Res2, aRnetGraphFile, lInputTensorsMap2, lOutputTensorNamesMap2);

  if ((Status_t::SUCCESS != status) || lOutputTensorNamesMap2.empty())
  {
    std::cout << "Failed to load net2" << std::endl;
    return {};
  }

  status = net_Res2->SetTargetHint(lCpuString);
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Set CPU target failed" << std::endl;
    return {};
  }

  status = net_Res2->Prepare();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net2 verification failed" << std::endl;
    return {};
  }

  // Load class labels
  std::vector<std::string> classLabels;
  LoadTxtFile(aSlimLabelsFile, classLabels);

  std::cout << "\nClassifying \'" << aImageFile << "\' ..." << std::endl;
  cv::Mat image = cv::imread(aImageFile);
  if (!image.data)
  {
    std::cout << "Could not open or find image: " << aImageFile << std::endl;
    return {};
  }
  if (Status_t::SUCCESS != resizeBilinearAndNormalizeFloat(image, &lInputTensorChild1, {123.675, 116.28, 103.53}, {0.01712475383, 0.01750700280, 0.01742919389}))
  {
    std::cout << "Failed to read " << aImageFile << std::endl;
    return {};
  }
  lInputTensor1->Flush();

  status = net_Res1->Run();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net1 execution failed" << std::endl;
    return {};
  }

  std::cout << "net1 run done: " << std::endl;

  lInputTensor2->CopyDataFrom(*lOutputTensorNamesMap1[aFirstGraphOutput]);

  status = net_Res2->Run();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net2 execution failed" << std::endl;
    return {};
  }
  std::cout << "net2 run done: " << std::endl;
  
  lOutputTensorNamesMap2[aSecondGraphOutput]->Invalidate();
  std::cout << lOutputTensorNamesMap2[aSecondGraphOutput]->Dims()[0] << "/" << lOutputTensorNamesMap2[aSecondGraphOutput]->Dims()[1] 
            << "/" << lOutputTensorNamesMap2[aSecondGraphOutput]->Dims()[2] << "/" << lOutputTensorNamesMap2[aSecondGraphOutput]->Dims()[3] << std::endl;  

  // Apply softmax on output tensor
  std::vector<float> result;
  if (true == isExtSoftmax)
  {
    SoftMax(*lOutputTensorNamesMap2[aSecondGraphOutput], result);
  }
  else
  {
    TensorToFloatList(*lOutputTensorNamesMap2[aSecondGraphOutput], result);
  }

  // Get top 5 result
  std::vector<std::pair<uint32_t, float>> results;
  processClassificationResults(result, results);
  

  std::cout << std::endl << "Top 5: " << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[0].first] << ", " << std::to_string(results[0].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[1].first] << ", " << std::to_string(results[1].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[2].first] << ", " << std::to_string(results[2].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[3].first] << ", " << std::to_string(results[3].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[4].first] << ", " << std::to_string(results[4].second)
            << std::endl;
  std::cout << std::endl;

  display_image(image, classLabels, results, flipDisplay);

  return lOutputTensorNamesMap1[aFirstGraphOutput];
}

std::unique_ptr<Tensor> case_resnet_target_split_quant( const std::string& aRnetGraphFile,
                                                        const std::string& aGraphInput,
                                                        const std::string& aSecondRnetGraphFile,
                                                        const std::string& aFirstGraphOutput,
                                                        const std::string& aFirstGraphOutNode,
                                                        const std::string& aSecondGraphOutput,
                                                        const std::string& aImageFile,
                                                        const std::string& aSlimLabelsFile,
                                                        const std::string& aTarget,
                                                        bool  isCPUOpt,
                                                        bool  isExtSoftmax,
                                                        bool  flipDisplay )
{
  std::cout << std::endl << "Running \'" << aRnetGraphFile << "\' graph for target " << aTarget << std::endl;
  
  // Creating a new workspace
  std::string lCpuString = isCPUOpt ? TargetType::CPU_OPT() : TargetType::CPU_REF();
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();
  lTargets[TargetType::APEX()]    = CreateApexTarget();
  if (isCPUOpt)
  {
    lTargets[TargetType::CPU_OPT()] = CreateCpuOptTarget();
  }

  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Create the two subgraphs
  auto net_Res1 = std::unique_ptr<Graph>(new Graph(*lWorkspace));
  auto net_Res2 = std::unique_ptr<Graph>(new Graph(*lWorkspace));

  // Configure input to first graph
  Tensor* lInputTensor1 = net_Res1->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT, TensorShape<TensorFormat_t::NHWC>{1, 32+6, 32+6, 3},
                       TensorLayout<TensorFormat_t::NHWC>())));
  
  lInputTensor1->Allocate(Allocation_t::OAL);
  lInputTensor1->SetQuantParams({QuantInfo(-4, 4)});

  Tensor lInputTensorChild1("input");
  lInputTensorChild1.Configure<>(DataType_t::SIGNED_8BIT, TensorShape<TensorFormat_t::NHWC>{1, 32, 32, 3});
  lInputTensorChild1.SetParent(lInputTensor1, {0, 3, 3, 0});

  // Load graph from protobuf model
  std::map<std::string, Tensor*> lInputTensorsMap1 = {{aGraphInput, lInputTensor1}};
  std::map<std::string, Tensor*> lOutputTensorNamesMap1;
  lOutputTensorNamesMap1[aFirstGraphOutput];
 
  Status_t status = LoadGraphFromONNXV2(*net_Res1, aRnetGraphFile, lInputTensorsMap1, lOutputTensorNamesMap1);

  if ((Status_t::SUCCESS != status) || lOutputTensorNamesMap1.empty())
  {
    std::cout << "Failed to load net1" << std::endl;
    return {};
  }
  if ("APEX" == aTarget)
  {
    status = net_Res1->SetTargetHint(TargetType::APEX());
    if (Status_t::SUCCESS != status)
    {
      std::cout << "Set APEX target failed" << std::endl;
      return {};
    }
  }
  else
  {
    status = net_Res1->SetTargetHint(lCpuString);
    if (Status_t::SUCCESS != status)
    {
      std::cout << "Set CPU target failed" << std::endl;
      return {};
    }
  }

  Tensor* lNetProbe = net_Res1->GetTensor(aFirstGraphOutNode);
  if (nullptr == lNetProbe)
  {
    std::cout << "tensor not found " << aFirstGraphOutNode << std::endl;
    return {};
  }

  // Ensure the graph tensor isn't allocated from the arena
  net_Res1->SetOutput(lNetProbe);
  status = net_Res1->Prepare();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net1 verification failed" << std::endl;
    return {};
  }

  const int32_t lOutCh  = lOutputTensorNamesMap1[aFirstGraphOutput]->Dim(TensorDim_t::OUTPUT_CHANNELS);
  const int32_t lBatch  = lOutputTensorNamesMap1[aFirstGraphOutput]->Dim(TensorDim_t::BATCH);
  const int32_t lHeight = lOutputTensorNamesMap1[aFirstGraphOutput]->Dim(TensorDim_t::HEIGHT);
  const int32_t lWidth  = lOutputTensorNamesMap1[aFirstGraphOutput]->Dim(TensorDim_t::WIDTH);

  Tensor* lInputTensor2 = net_Res2->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{lBatch, lHeight, lWidth, lOutCh},
                       TensorLayout<TensorFormat_t::NHWC>())));

  lInputTensor2->Allocate(Allocation_t::HEAP);

  std::map<std::string, Tensor*> lInputTensorsMap2 = {{aFirstGraphOutput, lInputTensor2}};

  std::map<std::string, Tensor*> lOutputTensorNamesMap2;
  lOutputTensorNamesMap2[aSecondGraphOutput];

  status = LoadGraphFromONNXV2(*net_Res2, aSecondRnetGraphFile, lInputTensorsMap2, lOutputTensorNamesMap2);

  if ((Status_t::SUCCESS != status) || lOutputTensorNamesMap2.empty())
  {
    std::cout << "Failed to load net2" << std::endl;
    return {};
  }

  status = net_Res2->SetTargetHint(lCpuString);
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Set CPU target failed" << std::endl;
    return {};
  }
  
  status = net_Res2->Prepare();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net2 verification failed" << std::endl;
    return {};
  }

  // Load class labels
  std::vector<std::string> classLabels;
  LoadTxtFile(aSlimLabelsFile, classLabels);

  std::cout << "\nClassifying \'" << aImageFile << "\' ..." << std::endl;
  cv::Mat image = cv::imread(aImageFile);
  if (!image.data)
  {
    std::cout << "Could not open or find image: " << aImageFile << std::endl;
    return {};
  }

  if (Status_t::SUCCESS != resizeBilinearAndNormalize(image, &lInputTensorChild1, {123.675, 116.28, 103.53}, {32.f/58.395f, 32.f/57.12f, 32.f/57.37f}))
  {
    std::cout << "Failed to read " << aImageFile << std::endl;
    return {};
  }
  lInputTensor1->Flush();

  //std::cout << net_Res1->Show(ShowLevel_t::VERBOSE) << std::endl;

  status = net_Res1->Run();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net1 execution failed" << std::endl;
    return {};
  }
  std::cout << "net1 run done: " << std::endl;

  lInputTensor2->CopyDataFrom(*lOutputTensorNamesMap1[aFirstGraphOutput]);

  status = net_Res2->Run();
  if (Status_t::SUCCESS != status)
  {
    std::cout << "Net2 execution failed" << std::endl;
    return {};
  }
  std::cout << "net2 run done: " << std::endl;
  
  lOutputTensorNamesMap2[aSecondGraphOutput]->Invalidate();
  std::cout << lOutputTensorNamesMap2[aSecondGraphOutput]->Dims()[0] << "/" << lOutputTensorNamesMap2[aSecondGraphOutput]->Dims()[1] 
            << "/" << lOutputTensorNamesMap2[aSecondGraphOutput]->Dims()[2] << "/" << lOutputTensorNamesMap2[aSecondGraphOutput]->Dims()[3] << std::endl;  

  // Apply softmax on output tensor
  std::vector<float> result;
  if (true == isExtSoftmax)
  {
    SoftMax(*lOutputTensorNamesMap2[aSecondGraphOutput], result);
  }
  else
  {
    TensorToFloatList(*lOutputTensorNamesMap2[aSecondGraphOutput], result);
  }

  // Get top 5 result
  std::vector<std::pair<uint32_t, float>> results;
  processClassificationResults(result, results);

  std::cout << std::endl << "Top 5: " << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[0].first] << ", " << std::to_string(results[0].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[1].first] << ", " << std::to_string(results[1].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[2].first] << ", " << std::to_string(results[2].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[3].first] << ", " << std::to_string(results[3].second)
            << std::endl;
  std::cout << std::right << std::setw(20) << classLabels[results[4].first] << ", " << std::to_string(results[4].second)
            << std::endl;
  std::cout << std::endl;

  std::unique_ptr<Tensor> lCopy = std::unique_ptr<Tensor>(
      Tensor::Create<>( "copy", DataType_t::FLOAT,
                        TensorShape<TensorFormat_t::NHWC>{  lNetProbe->Dim(TensorDim_t::BATCH), 
                                                            lNetProbe->Dim(TensorDim_t::HEIGHT), 
                                                            lNetProbe->Dim(TensorDim_t::WIDTH), 
                                                            lNetProbe->Dim(TensorDim_t::OUTPUT_CHANNELS) } ));

  lCopy->Allocate(Allocation_t::HEAP);
  lCopy->CopyDataFrom(*lNetProbe);

  display_image(image, classLabels, results, flipDisplay);

  return std::move(lCopy);

}

int case_resnet(  const std::string& aRnetGraphFile,
                  const std::string& aSecondRnetGraphFile,
                  const std::string& aImageFile,
                  const std::string& aSlimLabelsFile,
                  bool  isCPUOpt,
                  bool  isExtSoftmax,
                  bool  flipDisplay,
                  int   index )
{
  /* resnet18 pytorch version */
  std::string lGraphInput         = "input.1";
  std::string lFirstGraphOutput   = "16";
  std::string lFirstGraphOutNode  = "16";
  std::string lSecondGraphOutput  = "29";

#ifdef USE_ONLY_PYTORCH_VERSION
  if (index == 1)
#else
  if (index > 3)
  {
    /* resnet18v1 onnx model zoo version */
    lGraphInput         = "data";
    lFirstGraphOutput   = "resnetv15_pool1_fwd";
    lFirstGraphOutNode  = "resnetv15_pool1_fwd";
    lSecondGraphOutput  = "resnetv15_dense0_fwd";
  }

  if ((index == 1) || (index == 4))
#endif // !USE_ONLY_PYTORCH_VERSION
  {
    auto cpu_results = case_resnet_target(aRnetGraphFile, lGraphInput, aImageFile, aSlimLabelsFile, "CPU",  isCPUOpt, isExtSoftmax, flipDisplay);
    if (cpu_results.empty())
    {
      return -1;
    }
  }

#ifdef USE_ONLY_PYTORCH_VERSION
  if (index == 2)
#else
  if ((index == 2) || (index == 5))
#endif // !USE_ONLY_PYTORCH_VERSION
  {
    auto cpu_float_tensor = case_resnet_target_split(aRnetGraphFile, lGraphInput, lFirstGraphOutput, lFirstGraphOutNode, lSecondGraphOutput, aImageFile, aSlimLabelsFile, "CPU",  isCPUOpt, isExtSoftmax, flipDisplay);
    if (nullptr == cpu_float_tensor)
    {
      return -1;
    }
  }

#ifdef USE_ONLY_PYTORCH_VERSION
  if (index == 3)
#else
  if ((index == 3) || (index == 6))
#endif // !USE_ONLY_PYTORCH_VERSION
  {
    auto cpu_fixed_tensor = case_resnet_target_split_quant(aRnetGraphFile, lGraphInput, aSecondRnetGraphFile, lFirstGraphOutput, lFirstGraphOutNode, lSecondGraphOutput, aImageFile, aSlimLabelsFile, "CPU",  isCPUOpt, isExtSoftmax, flipDisplay);

    auto apex_fixed_tensor = case_resnet_target_split_quant(aRnetGraphFile, lGraphInput, aSecondRnetGraphFile, lFirstGraphOutput, lFirstGraphOutNode, lSecondGraphOutput, aImageFile, aSlimLabelsFile, "APEX",  isCPUOpt, isExtSoftmax, flipDisplay);

    if ((nullptr == cpu_fixed_tensor) || (nullptr == apex_fixed_tensor))
    {
      return -1;
    }
  }
  return 0;
}
