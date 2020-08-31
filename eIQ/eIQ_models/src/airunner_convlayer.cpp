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
#include <random>
#include "apex.h"
#include "airunner_public.hpp"
#include "airunner_node_config.hpp"
#include "airunner_public_apex.hpp"
#include <airunner_public_cpu.hpp>
#include <airunner_public_importer.hpp>
#include "airunner_convlayer.hpp"
#include "airunner_tensor_utils.hpp"
#include "airunner_tensor_range.hpp"

using namespace airunner;
namespace {
void RandomizeTensor(Tensor& aTensor, const int32_t aSeed)
{
  std::mt19937 slRand;
  if(aSeed > -1)
  {
    slRand.seed(aSeed);
  }
  std::uniform_int_distribution<int8_t> lDis(std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max());
  TensorRange<int8_t> lRange(aTensor);
  for(auto& val : lRange)
  {
    val = lDis(slRand);
  }
}

bool TensorEqual(const Tensor& aTensorA, const Tensor& aTensorB)
{
  TensorRange<int8_t, true> lRangeA(aTensorA);
  TensorRange<int8_t, true> lRangeB(aTensorB);
  auto lAIter = lRangeA.begin();
  for(const auto& val : lRangeB)
  {
    if(val != *lAIter)
    {
      return false;
    }
  }
  return true;
}

}


static int conv2d_net_construction (int                     aBatch,
                                    int                     akW,
                                    int                     akH,
                                    int                     aWidth,
                                    int                     aHeight,
                                    int                     aStrideW,
                                    int                     aStrideH,
                                    int                     aInputChannels,
                                    int                     aOutputChannels,
                                    int                     aGroup,
                                    PaddingConfig           aPadding,
                                    ActivationConfig        aActivation,
                                    int                     aFastProf )
{
  /* INPUT IMAGE */
  int lInputWidth  = aWidth;
  int lInputHeight = aHeight;
  int lInputChannel  = aInputChannels;

  /* OUTPUT IMAGE */
  int lOutputChannel = aOutputChannels;

  Status_t lStatus = Status_t::SUCCESS;

  printf("\n");
  printf("Input  (iN, iW, iH, iC       ): %4d %4d %4d %4d    \n", aBatch, lInputWidth, lInputHeight, lInputChannel);
  printf("Conv   (kW, kH, sW, sH, Group): %4d %4d %4d %4d %4d\n", akW, akH, aStrideW, aStrideH, aGroup );
  if(aActivation.mType == ActivationFunction_t::NONE)
    printf("Testing aActivation.mType: NONE\n");
  if(aActivation.mType == ActivationFunction_t::RELU)
    printf("Testing aActivation.mType: RELU\n");
  if(aActivation.mType == ActivationFunction_t::BRELU)
    printf("Testing aActivation.mType: BRELU\n");

  // Creating a new workspace
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();
#ifdef __aarch64__
  lTargets[TargetType::APEX()] = CreateApexTarget();
#endif
  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Create Input tensor, fill with random data
  std::unique_ptr<Tensor> lNetInputTensor = nullptr;
  if(aInputChannels <= 3)
  {
    lNetInputTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
      "NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
      TensorShape<TensorFormat_t::NHWC>{aBatch, lInputHeight, lInputWidth, lInputChannel}));
  }
  else 
  {
    lNetInputTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
      "NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
      TensorShape<TensorFormat_t::NHWCE0>{aBatch, lInputHeight, lInputWidth, lInputChannel},
      TensorLayout<TensorFormat_t::NHWCE0>(TensorDim_t::WIDTH, 2)));
  }
#ifdef __aarch64__
  lNetInputTensor->Allocate(Allocation_t::OAL);
#else
  lNetInputTensor->Allocate(Allocation_t::HEAP);
#endif  
  lNetInputTensor->SetQuantParams({QuantInfo(-6, 6)});
  RandomizeTensor(*lNetInputTensor.get(), 0);

  auto lCpuNetInputTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
      "NET_INPUT_TENSOR_COPY", DataType_t::SIGNED_8BIT,
      TensorShape<TensorFormat_t::NHWC>{aBatch, lInputHeight, lInputWidth, lInputChannel}));
  lCpuNetInputTensor->Allocate(Allocation_t::HEAP);
  lCpuNetInputTensor->SetQuantParams({QuantInfo(-6, 6)});
  lCpuNetInputTensor->CopyDataFrom(*lNetInputTensor.get());

  // Create Weight tensor, fill with random data
  auto lWeightTensor = std::unique_ptr<Tensor>(
      Tensor::Create<>("WEIGHT_TENSOR", DataType_t::SIGNED_8BIT,
                       TensorShape<TensorFormat_t::OIHW>{lOutputChannel, lInputChannel/aGroup, akH, akW}));
  lWeightTensor->Allocate(Allocation_t::HEAP);
  RandomizeTensor(*lWeightTensor.get(), 0);
  lWeightTensor->SetQuantParams({QuantInfo(-0.3, 0.8)});

  // Create bias tensor, fill with random data
  auto lBiasTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
      "BIAS_TENSOR", DataType_t::SIGNED_32BIT,
      TensorShape<TensorFormat_t::OIHW>{lOutputChannel, 1, 1, 1}));
  lBiasTensor->Allocate(Allocation_t::HEAP);
  RandomizeTensor(*lBiasTensor.get(), 0);
  lBiasTensor->SetQuantParams({QuantInfo(-0.3, 0.8)});

  // Build up the graph
  auto lGraph = std::unique_ptr<Graph>(new Graph(*lWorkspace));

  // Create output tensor
  auto outputTensor = lGraph->AddTensor("NET_OUTPUT_TENSOR");
  outputTensor->SetQuantParams({QuantInfo(-106, 106)});

  lGraph->AddNode(
      NodeFactory<ConvConfig>::Create(ConvConfig{{akH, akW}, {aStrideH, aStrideW}, {1, 1},  aGroup, {aPadding}, {aActivation}}),
      {lNetInputTensor.get(), lWeightTensor.get(), lBiasTensor.get()}, {outputTensor});
 
  // Run APEX
#ifdef __aarch64__
  lStatus = lGraph->SetTargetHint(TargetType::APEX());
#else
  lStatus = lGraph->SetTargetHint(TargetType::CPU_REF());
#endif  
  
  if(Status_t::SUCCESS != lStatus)
  {
    std::cout << "Set target failed" << std::endl;
    return -1;
  }
  lStatus = lGraph->Prepare();

  if(Status_t::SUCCESS != lStatus)
  {
    std::cout << "APEX Prepare() failed" << std::endl;
    return -1;
  }

  lNetInputTensor->Flush();
  outputTensor->Invalidate();
  lStatus = lGraph->Run();
  if(Status_t::SUCCESS != lStatus)
  {
    std::cout << "APEX Run() failed " << int(lStatus) << std::endl;
    return -1;
  }

  if(!aFastProf)
  {
    lNetInputTensor->Configure(TensorFormat_t::NHWC, lNetInputTensor->DataType(), lNetInputTensor->Dims(),
                            lNetInputTensor->Layout());

    lStatus = lNetInputTensor->Allocate(Allocation_t::HEAP);
    if(Status_t::SUCCESS != lStatus)
    {
      std::cout << "Tensor allocation failed" << std::endl;
      return -1;
    }

    lNetInputTensor->CopyDataFrom(*lCpuNetInputTensor.get());

    // Run reference for comparison
    auto lApexNetOutput = std::unique_ptr<Tensor>(new Tensor("APEX_OUTPUT"));
    lApexNetOutput->Configure(outputTensor->Format(), outputTensor->DataType(),
                              outputTensor->Dims(), outputTensor->Layout());
    lStatus = lApexNetOutput->Allocate(Allocation_t::HEAP);
    if(Status_t::SUCCESS != lStatus)
    {
      std::cout << "APEX output allocation failed" << std::endl;
      return -1;
    }

    outputTensor->Invalidate();
    lApexNetOutput->CopyDataFrom(*outputTensor);
    memset(outputTensor->DataPtr(), 0, outputTensor->Size());

    lStatus = lGraph->SetTargetHint(TargetType::CPU_REF());
    if(Status_t::SUCCESS != lStatus)
    {
      std::cout << "Could not set target to REF" << std::endl;
      return -1;
    }
    lStatus = lGraph->Prepare();
    if(Status_t::SUCCESS != lStatus)
    {
      std::cout << "Prepare failed " << int(lGraph->State()) << std::endl;
      return -1;
    }

    lNetInputTensor->Flush();
    outputTensor->Invalidate();
    lStatus = lGraph->Run();

    if(TensorEqual(*lApexNetOutput.get(), *outputTensor))
    {
      std::cout << "**************\n";
      std::cout << "TEST SUCCESS\n";
      std::cout << "**************" << std::endl;
      lStatus        = Status_t::SUCCESS;
    }
    else
    {
      std::cout << "**************\n";
      std::cout << "TEST FAILED\n";
      std::cout << "**************" << std::endl;
      lStatus        = Status_t::INTERNAL_ERROR;
    }
  }

//end:
  return lStatus == Status_t::SUCCESS ? 0 : 1;
}

/* depth wise 3x3 convolution */
static int depthconv_net_construction(int              aBatch,
                                      int              akW,
                                      int              akH,
                                      int              aWidth,
                                      int              aHeight,
                                      int              aStrideW,
                                      int              aStrideH,
                                      int              aInputChannels,
                                      PaddingConfig    aPadding,
                                      ActivationConfig aActivation,
                                      int              aFastProf)
{
  /* INPUT IMAGE */
  int lInputWidth  = aWidth;
  int lInputHeight = aHeight;
  int lInputChannel  = aInputChannels;

  /* OUTPUT IMAGE */
  int lOutputChannel = aInputChannels;

  /* KERNEL */
  int lFilterWidth  = 3;
  int lFilterHeight = 3;

  Status_t lStatus = Status_t::SUCCESS;


  printf("\n");
  printf("Input     (iN, iW, iH, iC): %4d %4d %4d %4d\n", aBatch, lInputWidth, lInputHeight, lInputChannel);
  printf("DEPTHConv (kW, kH, sW, sH): %4d %4d %4d %4d\n", akW, akH, aStrideW, aStrideH );
  if(aActivation.mType == ActivationFunction_t::NONE)
    printf("Testing aActivation.mType: NONE\n");
  if(aActivation.mType == ActivationFunction_t::RELU)
    printf("Testing aActivation.mType: RELU\n");
  if(aActivation.mType == ActivationFunction_t::BRELU)
    printf("Testing aActivation.mType: BRELU\n");

  // Creating a new workspace
  std::map<std::string, std::unique_ptr<Target>> lTargets;
  lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();
#ifdef __aarch64__
  lTargets[TargetType::APEX()] = CreateApexTarget();
#endif  

  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));
  
  {
      // Create Input tensor, fill with random data
      auto lNetInputTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
          "NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
          TensorShape<TensorFormat_t::NHWCE0>{aBatch, lInputHeight, lInputWidth, lInputChannel},
          TensorLayout<TensorFormat_t::NHWCE0>(TensorDim_t::WIDTH, 2)));
      lNetInputTensor->Allocate(Allocation_t::OAL);
      RandomizeTensor(*lNetInputTensor.get(), 0);
      lNetInputTensor->SetQuantParams({QuantInfo(-6, 6)});

      // Create Weight tensor, fill with random data
      auto lWeightTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
          "WEIGHT_TENSOR", DataType_t::SIGNED_8BIT,
          TensorShape<TensorFormat_t::OIHW>{1, lInputChannel, lFilterHeight, lFilterWidth}));
      lWeightTensor->Allocate(Allocation_t::HEAP);
      RandomizeTensor(*lWeightTensor.get(), 0);
      lWeightTensor->SetQuantParams({QuantInfo(-0.3, 0.8)});

      // Create bias tensor, fill with random data
      auto lBiasTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
          "BIAS_TENSOR", DataType_t::SIGNED_32BIT,
          TensorShape<TensorFormat_t::OIHW>{lOutputChannel, 1, 1, 1}));
      lBiasTensor->Allocate(Allocation_t::HEAP);
      RandomizeTensor(*lBiasTensor.get(), 0);
      lBiasTensor->SetQuantParams({QuantInfo(-0.3, 0.8)});

      // Build up the graph
      auto lGraph = std::unique_ptr<Graph>(new Graph(*lWorkspace));

      // Create output tensor
      auto outputTensor = lGraph->AddTensor("NET_OUTPUT_TENSOR");
      outputTensor->SetQuantParams({QuantInfo(0, 8.0)});

      lGraph->AddNode(
             NodeFactory<DepthConvConfig>::Create(
                DepthConvConfig{{akH, akW}, {aStrideH, aStrideW}, {1, 1}, std::move(aPadding), std::move(aActivation)}),
              {lNetInputTensor.get(), lWeightTensor.get(), lBiasTensor.get()}, {outputTensor});
     
      // Run APEX
#ifdef __aarch64__
      lStatus = lGraph->SetTargetHint(TargetType::APEX());
#else
      lStatus = lGraph->SetTargetHint(TargetType::CPU_REF());
#endif 

      if(Status_t::SUCCESS != lStatus)
      {
        std::cout << "Set APEX target failed" << std::endl;
        goto end;
      }
      lStatus = lGraph->Prepare();

      if(Status_t::SUCCESS != lStatus)
      {
        std::cout << "APEX Prepare() failed" << std::endl;
        goto end;
      }

      lNetInputTensor->Flush();
      outputTensor->Invalidate();

      lStatus = lGraph->Run();
      if(Status_t::SUCCESS != lStatus)
      {
        std::cout << "APEX Run() failed " << int(lStatus) << std::endl;
        goto end;
      }

      if(!aFastProf)
      {
        // Run reference for comparison
        Tensor lApexNetOutput("APEX_OUTPUT");
        lApexNetOutput.Configure(outputTensor->Format(), outputTensor->DataType(),
                                 outputTensor->Dims(), 
                                 outputTensor->Layout());
        lStatus = lApexNetOutput.Allocate(Allocation_t::HEAP);
        if(Status_t::SUCCESS != lStatus)
        {
          std::cout << "APEX output allocation failed" << std::endl;
          goto end;
        }
        
        outputTensor->Invalidate();
        lApexNetOutput.CopyDataFrom(*outputTensor);
        memset(outputTensor->DataPtr(), 0, outputTensor->Size());

        lStatus = lGraph->SetTargetHint(TargetType::CPU_REF());
        if(Status_t::SUCCESS != lStatus)
        {
          std::cout << "Could not set target to REF" << std::endl;
          goto end;
        }
        lStatus = lGraph->Prepare();
        if(Status_t::SUCCESS != lStatus)
        {
          std::cout << "Prepare failed " << int(lGraph->State()) << std::endl;
          goto end;
        }

        lNetInputTensor->Flush();
        outputTensor->Invalidate();
        lStatus = lGraph->Run();

        if(TensorEqual(lApexNetOutput, *outputTensor))
        {
          std::cout << "**************\n";
          std::cout << "TEST SUCCESS\n";
          std::cout << "**************" << std::endl;
        }
        else
        {
          std::cout << "**************\n";
          std::cout << "TEST FAILED\n";
          std::cout << "**************" << std::endl;
          lStatus        = Status_t::INTERNAL_ERROR;
        }
      }
  }

end:
  return lStatus == Status_t::SUCCESS ? 0 : 1;
}





#if 1

 int pb_model(){




/* Create a workspace:
* The workspace constructor takes as a parameter a dictionary with string keys that map to
* unique_ptr of target objects. The workspace takes ownership of the target objects.
* In this example we want to have access to reference CPU and APEX targets. We could choose anything
* for our target instance, since we are only adding 1 target of each type we can just use the target
* type string.
*/
std::map<std::string, std::unique_ptr<Target>> lTargets;
lTargets[TargetType::CPU_REF()] = CreateCpuRefTarget();
lTargets[TargetType::APEX()] = CreateApexTarget();
Workspace lWorkspace(std::move(lTargets));
/*
* Create a graph:
* A graph is a collection of nodes linked by tensors. A graph is associated with a workspace and will
only
* use resources from the targets in that workspace.
*/

Graph lGraph(lWorkspace);

//auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::move(lTargets)));

  // Create the two subgraphs
//auto net_mobile      = std::unique_ptr<Graph>(new Graph(*lWorkspace));

/*
* Create an input tensor:
*/
Tensor* lNetInput1 = lGraph.AddTensor(
Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT, TensorShape<TensorFormat_t::NHWC>{1, 2,
3, 1}));


Tensor* lNetInput2 = lGraph.AddTensor(
Tensor::Create<>("NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT, TensorShape<TensorFormat_t::NHWC>{1, 2,
3, 1}));

/*
* Set the quantization range of the input tensor.
*/

//lNetInput.SetQuantParams({{-1, 1}}); // 这边暂时没有相关必要性

/*
* Populate graph from a tensorflow protobuf.
* The input tensor is connected to the first node in the graph and pointers to the output tensors of the
output nodes are returned.
*/
std::vector<Tensor*> lNetOutputs;
//LoadNetFromTensorFlow(lGraph, "concat_model.pb", lNetInput1,lNetOutputs);

Status_t status;
status = LoadNetFromTensorFlow(lGraph, "data/concat_model.pb", lNetInput1,lNetOutputs);
if(Status_t::SUCCESS != status || lNetOutputs.empty())
{
  std::cout << "Failed to load net" << std::endl;
  return {};
}

else 
  std::cout << "success to load net" << std::endl;


/*
* Set the target hint:
* When the graph is associating a node with a target it will attempt to use the hinted target. If that
fails it will
* fall back to another target in the workspace.
* Note: application code can manually associate a node with a target by calling
Node::SetTarget(std::shared_ptr<Target>).
* In this case the target does not need to be a part of the workspace. The graph will not attempt to
override an explicit
* node target selection.
*/
lGraph.SetTargetHint(TargetType::APEX());
/*
* Prepare the graph for execution:
* The graph will assign each node a target, verify that nodes have all of their required inputs and that
those inputs are compatible,
* and will allocate the intermediate tensors from the graph.
*/
lGraph.Prepare();
/*
* Run the graph:
* Assumes you have loaded data into the input tensor
*/
lGraph.Run();
/*
* Use the data stored in the output tensor
*/
for(const auto& tensor : lNetOutputs)
{
// Do something
}



  
}

#endif 










int conv_layer_test(int checkRef)
{

  pb_model();


  int lRetVal = 0;

  std::cout << std::endl;
  std::cout << "////////////////////////////////////////////////////\n";
  std::cout << "//////////// conv_layer_test /////////////\n";
  std::cout << "////////////////////////////////////////////////////" << std::endl;

  /* aBatch, akW, akH, aWidth, aHeight, aStrideW, aStrideH, aInputChannels, aOutputChannels, aGroup, aPadding, aActivation,      checkRef              */
  /*  N          K         Input               Stride            IC              OC            group, pad,     activation,      checkResultWithCRef    */
  //1x1s1
  lRetVal += conv2d_net_construction(5, 1, 1, 1, 56, 1, 1, 120, 30, 3, {PaddingScheme_t::SAME, 0, 0, 0, 0}, {ActivationFunction_t::NONE, 0.0, 0.0}, checkRef); // Conv_kh1_kw1_sh1_sw1_ph0_pw0_g3_#out30_5_120_56_1
  

  if(lRetVal == 0)
  {
    printf("==================================\n");
    printf("group conv_layer_test SUCCESS \n");
    printf("==================================\n");
    printf("===========================\n");
  }
  else
  {
    printf("==================================\n");
    printf("group conv_layer_test FAILED \n");
    printf("==================================\n");
  }

  return lRetVal;
}
