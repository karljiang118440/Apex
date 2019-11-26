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

using Clock = std::chrono::high_resolution_clock;
using namespace airunner;

template<typename T> std::vector<std::pair<int, float>> processResults(T* aResults, int aNumResults)
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

std::vector<std::pair<int, float>> case_mobilenet_target( const std::string& aMnetGraph, 
                                                          const std::string& aResultGraph, 
                                                          const std::string& aImageFile, 
                                                          const std::string& aSlimLabelsFile, 
                                                          const std::string& target)
{
#ifndef _WINDOWS
  std::cout << "Running MobileNet graph for "<< target << std::endl;

   Status_t status;

  // Creating a new workspace  
  TargetInfo lRefInfo = TargetInfo();
  ApexTargetInfo lApexInfo = ApexTargetInfo();
  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::map<std::string, TargetInfo*>{
      {target::REF, &lRefInfo}, {target::APEX, &lApexInfo}}));

  // Create and populate the net
  auto net_mobile = std::unique_ptr<Graph>(new Graph (lWorkspace.get())); 
  auto netFixedToFloat = std::unique_ptr<Graph>(new Graph (lWorkspace.get()));
  

  Tensor* lApexNetInput  = net_mobile->AddTensor(std::unique_ptr<Tensor>(Tensor::Create<>(
                        "NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
                        TensorShape<TensorFormat_t::NHWC>{1, 224, 224,3},
                        TensorLayout<TensorFormat_t::NHWC>())));

  lApexNetInput->SetQuantParams({QuantInfo(-1, 0)});
  lApexNetInput->Allocate(Allocation_t::OAL);


  // Integer output of the aMnetGraph except the last 3 layers. 
  std::vector<Tensor*> fixedOutput;
  
  status = LoadNetFromTensorFlow(*net_mobile, aMnetGraph, lApexNetInput, fixedOutput);
  if(Status_t::SUCCESS != status || fixedOutput.empty()){
    std::cout << "Failed to load net" << std::endl;
    return {};
  }
  if (target == "APEX"){
    status = net_mobile->SetTargetHint(target::APEX);
    if(Status_t::SUCCESS != status){
      std::cout << "Set APEX target failed" << std::endl;
      return{};
    }
  }
  else{
    status = net_mobile->SetTargetHint(target::REF); 
    if(Status_t::SUCCESS != status){
      std::cout << "Set CPU target failed" << std::endl;
      return {};
    }
  }

  status = net_mobile->Prepare();   
  if (Status_t::SUCCESS != status){
    std::cout << "Net verification failed" << std::endl;
    return {};
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
    return {};
  }

  status = netFixedToFloat->SetTargetHint(target::REF);
  if(Status_t::SUCCESS != status){
    std::cout << "Set target failed" << std::endl;
    return {};
  }

  status = netFixedToFloat->Prepare(); 
  if (Status_t::SUCCESS != status){
    std::cout << "Net verification failed" << std::endl;
    return {};
  }
  // Load class labels
  std::ifstream labelfile(aSlimLabelsFile);
  std::vector<std::string> classLabels;
  std::string line;
  
  while(std::getline(labelfile, line)){
      line.pop_back();
      classLabels.push_back(line);
  }
  
  std::string imagePath = aImageFile;
 
  float* result = (float*)malloc(sizeof(float) * 1001);

  std::cout << "Classifying: " << imagePath << std::endl;

  if (-1 == ReadImageToTensor(imagePath, lApexNetInput, nullptr, 128, 128, 0))
  {
    std::cout << "Failed to read " << imagePath << std::endl;
    lApexNetInput->Flush();
    Output[0]->Invalidate();
    return {};
  }

  lApexNetInput->Flush();
  fixedOutput[0]->Invalidate();

  // Run image through parsed model
  status = net_mobile->Run();
  if(status != Status_t::SUCCESS){
    std::cout << "Net execution failed" << std::endl;
    return {};
  }
  
  status = floatInterm->CopyDataFrom(*fixedOutput[0]);
  if(status != Status_t::SUCCESS){
    std::cout << "Copy failed" << std::endl;
    return {};
  }
  
  status = netFixedToFloat->Run();
  if(status != Status_t::SUCCESS){
    std::cout << "Fixed to float net execution failed" << std::endl;
    return {};
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

  io_FrameOutput_t lFrameOutput;
  lFrameOutput.Init(DISPLAY_SCENE_WIDTH, DISPLAY_SCENE_HEIGHT, io::IO_DATA_DEPTH_08, io::IO_DATA_CH3);
  DisplayImage(lFrameOutput, imagePath, classLabels, results); 
  sleep(5);
  return results;
#endif

  return{};
}

int case_mobilenet(const std::string& aMnetGraph, const std::string& aResultGraph, const std::string& aImageFile,const std::string& aSlimLabelsFile){
  std::ifstream infile(aImageFile);
  if(!infile.good())
  {
    std::cout << "Failed to read: " << aImageFile << std::endl;
    return -1;
  }

  auto results_apex = case_mobilenet_target(aMnetGraph, aResultGraph, aImageFile, aSlimLabelsFile, "APEX");
  auto results_cpu = case_mobilenet_target(aMnetGraph, aResultGraph, aImageFile, aSlimLabelsFile, "CPU");

  if(results_apex.empty() || results_cpu.empty())
  {
    std::cout << "Error: Empty tensor." << std::endl;
    return -1;
  }

#ifndef _WINDOWS
  if(results_apex.size() != results_cpu.size()){
    std::cout << "Results vector size mismatch!" << std::endl;
    return -1;
  }
#endif

  std::cout << "MobileNet Test APEX/CPU fixed point match Successful" << std::endl;

  return 0;
}