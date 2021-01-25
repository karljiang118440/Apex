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

#include <airunner_tensor_range.hpp>
#include "test_cases.hpp"
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>


using namespace airunner;

void display_image(cv::Mat image, bool flipDisplay)
{
#ifdef __aarch64__
  io_FrameOutput_t lFrameOutput;
  lFrameOutput.Init(DISPLAY_SCENE_WIDTH, DISPLAY_SCENE_HEIGHT, io::IO_DATA_DEPTH_08, io::IO_DATA_CH3);
  cv::Mat resizedImage;
  cv::resize(image, resizedImage, cv::Size(DISPLAY_SCENE_WIDTH, DISPLAY_SCENE_HEIGHT), 0, 0, cv::INTER_NEAREST);
  if(flipDisplay)
  {
    cv::flip(resizedImage, resizedImage, -1);
  }
  lFrameOutput.PutFrame(resizedImage.data, false);
  sleep(2);
#endif
}

void TinyYoloPostProcessing(const std::string& aYoloGraph, Tensor* input0, Tensor* input1, int32_t height, int32_t width,
                            std::vector<float32_t> &out_boxes, std::vector<float32_t> &out_scores, std::vector<float32_t> &out_classes)
{
  const std::string lSplitGraphPath = "data/airunner/yolov3-tiny_subgraph2.onnx";
  std::vector<std::string> lInputNames  = {"convolution_output", "convolution_output1"}; 
  std::vector<std::string> lOutputNames = {"yolonms_layer_1", "yolonms_layer_1:1", "yolonms_layer_1:2"};
  LoadSubGraphFromONNX(aYoloGraph, lInputNames, lOutputNames, lSplitGraphPath);




const std::string aGraphName3 = "data/airunner/yolov3-tiny_10000.onnx";
std::vector<std::string> aInputName30 = {"input_1"};
std::vector<std::string> aOutputName30 = {"convolution_output1", "convolution_output"};
const std::string asubGraphName30 = "data/airunner/tiny_yolov3_subgraph1_10000.onnx";
Status_t lStatus = LoadSubGraphFromONNX(aGraphName3, aInputName30, aOutputName30, asubGraphName30);





  Ort::SessionOptions session_options;
  session_options.SetIntraOpNumThreads(1);
  session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
  
  Ort::Env env;
  Ort::Session session(env, lSplitGraphPath.c_str(), session_options);
  Ort::AllocatorWithDefaultOptions allocator;

  size_t num_input_nodes = session.GetInputCount();
  std::vector<const char*> input_node_names(num_input_nodes);
  printf("Number of inputs = %zu\n", num_input_nodes);

  for (int32_t i = 0; i < num_input_nodes; i++) {
    // print input node names
    char* input_name = session.GetInputName(i, allocator);
    printf("Input %d : name=%s\n", i, input_name);
    input_node_names[i] = input_name;
  }

  // Create onnxruntime input tensors
  std::vector<Ort::Value> input_tensors;
  auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  size_t input0_size = 255 * 26 * 26;
  std::vector<int64_t> input0_dims = {1, 255, 26, 26};
  std::vector<float32_t> input0_values(input0_size);
  float32_t* input0_pointer = input0_values.data();
  TensorRange<float32_t> lRange0(*input0);
  lRange0.IterateAs({1, 2, 0, 3});
  for(const auto val : lRange0)
  {
    *input0_pointer++ = val;
  }
  input_tensors.emplace_back(Ort::Value::CreateTensor<float32_t>(memory_info, input0_values.data(), input0_size, input0_dims.data(), 4));

  size_t input1_size = 255 * 13 * 13;
  std::vector<int64_t> input1_dims = {1, 255, 13, 13};
  std::vector<float32_t> input1_values(input1_size);
  float32_t* input1_pointer = input1_values.data();
  TensorRange<float32_t> lRange1(*input1);
  lRange1.IterateAs({1, 2, 0, 3});
  for(const auto val : lRange1)
  {
    *input1_pointer++ = val;
  }
  input_tensors.emplace_back(Ort::Value::CreateTensor<float32_t>(memory_info, input1_values.data(), input1_size, input1_dims.data(), 4));

  std::vector<float32_t> shape_values = {height, width}; // h x w
  std::vector<int64_t> shape_dims = {1, 2};
  input_tensors.emplace_back(Ort::Value::CreateTensor<float32_t>(memory_info, shape_values.data(), shape_values.size(), shape_dims.data(), 2));

  // Run postprocessing
  std::vector<const char*> output_node_names = {"yolonms_layer_1", "yolonms_layer_1:1", "yolonms_layer_1:2"};

  // score model & input tensor, get back output tensor
  auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_node_names.data(), input_tensors.data(), input_tensors.size(), output_node_names.data(), output_node_names.size());

  // std::cout << output_tensors.size() << std::endl;
  assert(output_tensors.size() == 3 && output_tensors[2].IsTensor());

  // // Get pointer to output tensor float32_t values
  float32_t* boxes = output_tensors[0].GetTensorMutableData<float32_t>();
  float32_t* scores = output_tensors[1].GetTensorMutableData<float32_t>();
  int32_t* indices = output_tensors[2].GetTensorMutableData<int32_t>();

  Ort::TensorTypeAndShapeInfo index_shape_info = output_tensors[2].GetTensorTypeAndShapeInfo();
  Ort::TensorTypeAndShapeInfo score_shape_info =output_tensors[1].GetTensorTypeAndShapeInfo();
  Ort::TensorTypeAndShapeInfo boxes_shape_info =output_tensors[0].GetTensorTypeAndShapeInfo();

  

  for(int32_t i = 0; i < index_shape_info.GetShape()[1]; ++i)
  {
    out_classes.emplace_back(indices[index_shape_info.GetShape()[2] * i + 1]);
    out_scores.emplace_back(scores[indices[index_shape_info.GetShape()[2] * i + 1] * score_shape_info.GetShape()[2] 
                                  + indices[index_shape_info.GetShape()[2] * i + 2]]);

    float32_t* boxesCords = &(boxes[indices[index_shape_info.GetShape()[2] * i + 2] * boxes_shape_info.GetShape()[2]]);
    out_boxes.emplace_back(boxesCords[0]);
    out_boxes.emplace_back(boxesCords[1]);
    out_boxes.emplace_back(boxesCords[2]);
    out_boxes.emplace_back(boxesCords[3]);
  }

  
}

inline std::vector<Tensor*> case_tiny_yolo_target(const std::string& aYoloGraph,
                                                  const std::string& aImageFile,
                                                  const std::string& aLabels,
                                                  const std::string& target,
                                                  bool aFixed,
                                                  bool aFlip)
{
  std::cout << "Running Yolo graph for " << target << std::endl;
  Status_t status;

  // Creating a new workspace
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();
  lTargets[TargetType::CPU_OPT()] = CreateCpuOptTarget();

  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Configure input to graph
  auto net_Yolo = std::unique_ptr<Graph>(new Graph(*lWorkspace));

  Tensor* lNetInput = net_Yolo->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 416, 416, 3},
                       TensorLayout<TensorFormat_t::NHWC>())));
  // lNetInput->SetQuantParams({QuantInfo(-1, 0)});
  lNetInput->Allocate(Allocation_t::HEAP);

  // Load graph from protobuf model
  std::map<std::string, Tensor*> lInputTensors = {{"input_1", lNetInput}};
  std::map<std::string, Tensor*> lOutputTensors;
  lOutputTensors["convolution_output"];
  lOutputTensors["convolution_output1"];

  status = LoadGraphFromONNXV2(*net_Yolo, aYoloGraph, lInputTensors, lOutputTensors);
  if(Status_t::SUCCESS != status || lOutputTensors.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }
  std::cout << "done loading graph" << std::endl;
  if(target == "APEX")
  {
    status = net_Yolo->SetTargetHint(TargetType::APEX());

    if(Status_t::SUCCESS != status)
    {
      std::cout << "Set target failed" << std::endl;
      return {};
    }
  }
  else
  {
    status = net_Yolo->SetTargetHint(TargetType::CPU_OPT());

    if(Status_t::SUCCESS != status)
    {
      std::cout << "Set target failed" << std::endl;
      return {};
    }
  }
  status = net_Yolo->Prepare();

  if(Status_t::SUCCESS != status)
  {
    std::cout << " Net verification failed" << std::endl;
    return {};
  }

  std::cout << "Detecting objects for: " << aImageFile << std::endl;
  cv::Mat outputImage = cv::imread(aImageFile);
  if(!outputImage.data)
  {
    std::cout << "SSD: could not open or find image" << std::endl;
    return {};
  }
  letterboxResizeBicubicAndNormalizeFloat(outputImage, lNetInput, {0}, {1 / 255.});
  lNetInput->Flush();

  // 2. Run preprocessed img through Yolo to produce raw class and box prediction results
  status = net_Yolo->Run();
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Net verification failed" << std::endl;
    return {};
  }

  Tensor* output0 = net_Yolo->GetTensor("convolution_output");
  Tensor* output1 = net_Yolo->GetTensor("convolution_output1");

  if(!output0 || !output1)
  {
    std::cout << "Net outputs not found " << std::endl;
    return {};
  }

  std::vector<float32_t> out_boxes, out_scores, out_classes;
  TinyYoloPostProcessing(aYoloGraph, output0, output1, outputImage.rows, outputImage.cols,
                          out_boxes, out_scores, out_classes );
  // Expected Results
  // class index: 16
  // score: 0.914906
  // box coords: 22.397  27.0156  572.919  342.706
  // class index: 16
  // score: 0.672506
  // box coords: 61.5501  400.928  592.802  968.626
  for (int32_t i = 0; i < out_scores.size(); ++i)
  {
    std::cout << "class index: " << out_classes[i] << std::endl;
    std::cout << "score: " << out_scores[i] << std::endl;
    std::cout << "box coords: " << out_boxes[i*4] << "  " << out_boxes[i*4+1] << "  " << out_boxes[i*4+2] << "  " << out_boxes[i*4+3] << std::endl;
  }
  std::vector<std::pair<uint32_t, float>> results;
  std::vector<BoundBox>boxes(out_scores.size());
  for (int32_t i = 0; i < out_scores.size(); ++i)
  {
    boxes[i].classLabel = out_classes[i];
    boxes[i].classScore = out_scores[i];
    boxes[i].boxCords.minY = out_boxes[i*4];
    boxes[i].boxCords.minX = out_boxes[i*4+1];
    boxes[i].boxCords.maxY = out_boxes[i*4+2];
    boxes[i].boxCords.maxX = out_boxes[i*4+3];
  }
  DrawBoundingBoxes(boxes, outputImage, results);
  display_image(outputImage, aFlip);

  return {};
}

inline std::vector<Tensor*> case_tiny_yolo_quant(const std::string& aYoloGraph,
                                                  const std::string& aImageFile,
                                                  const std::string& aLabels,
                                                  const std::string& target,
                                                  bool aFixed,
                                                  bool aFlip)
{
  std::cout << "Running Quant Yolo graph for " << target << std::endl;
  Status_t status;

  // Creating a new workspace
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();
  lTargets[TargetType::APEX()] = CreateApexTarget();

  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Configure input to graph
  auto net_Yolo = std::unique_ptr<Graph>(new Graph(*lWorkspace));

  auto lFloatInput = std::unique_ptr<Tensor>(
      Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 416, 416, 3},
                       TensorLayout<TensorFormat_t::NHWC>()));
  lFloatInput->Allocate(Allocation_t::HEAP);

  Tensor* lNetInput = net_Yolo->AddTensor(std::unique_ptr<Tensor>(
      Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT, TensorShape<TensorFormat_t::NHWC>{1, 416, 416, 3},
                       TensorLayout<TensorFormat_t::NHWC>())));
  lNetInput->SetQuantParams({QuantInfo(-1, 1)});
  lNetInput->Allocate(Allocation_t::OAL);

  // Load graph from protobuf model
  std::vector<Tensor*> lOutputTensors;
  status = LoadGraphFromONNX(*net_Yolo, aYoloGraph, lNetInput, lOutputTensors);
  if(Status_t::SUCCESS != status || lOutputTensors.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }
  status = net_Yolo->SetTargetHint(TargetType::APEX());

  if(Status_t::SUCCESS != status)
  {
    std::cout << "Set target failed" << std::endl;
    return {};
  }

  status = net_Yolo->Prepare();

  if(Status_t::SUCCESS != status)
  {
    std::cout << " Net verification failed" << std::endl;
    return {};
  }

  std::cout << "Detecting objects for: " << aImageFile << std::endl;
  cv::Mat outputImage = cv::imread(aImageFile);
  if(!outputImage.data)
  {
    std::cout << "SSD: could not open or find image" << std::endl;
    return {};
  }

  letterboxResizeBicubicAndNormalizeFloat(outputImage, lFloatInput.get(), {0}, {1 / 255.});

  lNetInput->CopyDataFrom(*lFloatInput);
  lNetInput->Flush();

  // 2. Run preprocessed img through Yolo to produce raw class and box prediction results
  status = net_Yolo->Run();
  if(Status_t::SUCCESS != status)
  {
    std::cout << "Net verification failed" << std::endl;
    return {};
  }

  std::cout << "Done graph execution" << std::endl;
  
  Tensor* output0 = net_Yolo->GetTensor("convolution_output");
  Tensor* output1 = net_Yolo->GetTensor("convolution_output1");

  if(!output0 || !output1)
  {
    std::cout << "Net outputs not found " << std::endl;
    return {};
  }

  auto output0_float = std::unique_ptr<Tensor>(
      Tensor::Create<>("output_0", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 26, 26, 255},
                       TensorLayout<TensorFormat_t::NHWC>()));
  output0_float->Allocate(Allocation_t::HEAP);
  output0_float->CopyDataFrom(*output0);

  auto output1_float = std::unique_ptr<Tensor>(
      Tensor::Create<>("output_1", DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, 13, 13, 255},
                       TensorLayout<TensorFormat_t::NHWC>()));
  output1_float->Allocate(Allocation_t::HEAP);
  output1_float->CopyDataFrom(*output1);

  std::cout << "Postprocessing" << std::endl;
  
  std::vector<float32_t> out_boxes, out_scores, out_classes;
  TinyYoloPostProcessing("data/airunner/yolov3-tiny.onnx", output0_float.get(), output1_float.get(), 
                         outputImage.rows, outputImage.cols, out_boxes, out_scores, out_classes);
  // Expected results
  // class index: 16
  // score: 0.904829
  // box coords: 32.3077  16.6555  567.239  348.963
  for (int32_t i = 0; i < out_scores.size(); ++i)
  {
    std::cout << "class index: " << out_classes[i] << std::endl;
    std::cout << "score: " << out_scores[i] << std::endl;
    std::cout << "box coords: " << out_boxes[i*4] << "  " << out_boxes[i*4+1] << "  " << out_boxes[i*4+2] << "  " << out_boxes[i*4+3] << std::endl;
  }
  std::vector<std::pair<uint32_t, float>> results;
  std::vector<BoundBox>boxes(out_scores.size());
  for (int32_t i = 0; i < out_scores.size(); ++i)
  {
    boxes[i].classLabel = out_classes[i];
    boxes[i].classScore = out_scores[i];
    boxes[i].boxCords.minY = out_boxes[i*4];
    boxes[i].boxCords.minX = out_boxes[i*4+1];
    boxes[i].boxCords.maxY = out_boxes[i*4+2];
    boxes[i].boxCords.maxX = out_boxes[i*4+3];
  }
  DrawBoundingBoxes(boxes, outputImage, results);
  display_image(outputImage, aFlip);

  return {};
}

int32_t case_tiny_yolo(const std::string& aYoloGraph,
                   const std::string& aImageFile,
                   const std::string& aSlimLabelsFile,
                   bool aFixed,
                   bool aFlip)
{
#ifndef _WINDOWS
  // Run CPU
  if(aFixed)
  {
    std::vector<Tensor*> cpuOutput =
      case_tiny_yolo_quant(aYoloGraph, aImageFile, aSlimLabelsFile, "APEX", aFixed, aFlip);
  }
  else
  {
    std::vector<Tensor*> cpuOutput =
      case_tiny_yolo_target(aYoloGraph, aImageFile, aSlimLabelsFile, "CPU", aFixed, aFlip);
  }

#endif
  return 0;
}
