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

/* // logs: 

1、用来测试其他的接口函数，如 Pooling -- 20200731



*/


#include <random>
#include "apex.h"
#include "airunner_public.hpp"
#include "airunner_node_config.hpp"
#include "airunner_public_apex.hpp"
#include <airunner_public_cpu.hpp>
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

void Fixedval2Tensor(Tensor& aTensor, const int32_t aSeed)
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
    val = aSeed;
  }
}

/* // 自己定义 Tensor 值的打印 -- 2020.07.23


*/
void TensorValPrint(Tensor& aTensor1)
{
  TensorRange<int8_t> lRange(aTensor1);
  for(auto& val : lRange)
  {
    //printf("aTensor1.val = %s,val = %d \n", aTensor1,val);
    //printf("name = %s \n",Tensor& aTensor1);
    printf("val = %d \n",val);

    //std::cout << "Tensor: " << std::string& aTensor1 << std::endl;

  }
}



void TensorValPrint_int8(Tensor& aTensor1)
{

  // print some values
  TensorRange<int8_t> lTensorRange(aTensor1);
  auto it = lTensorRange.begin();
  for (int32_t i = 10; i < 20; ++i)
  {
    //std::cout << *it++ << std::endl;
    printf("Tensor.val = %d \n",*it++);
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


/*
2020.08.05

copydatafrom api

能够对 tensor 中的数据进行复制

*/

//*********
   TensorValPrint_int8(*lNetInputTensor.get());
   std::unique_ptr<Tensor> lNetInputTensor_cpy = nullptr;

    lNetInputTensor_cpy = std::unique_ptr<Tensor>(Tensor::Create<>(
      "NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
      TensorShape<TensorFormat_t::NHWC>{aBatch, lInputHeight, lInputWidth, lInputChannel}));

    lNetInputTensor_cpy->CopyDataFrom(*lNetInputTensor.get());

    TensorValPrint(*lNetInputTensor_cpy.get());
//*************
//*************



/*
2020.08.05

copudatafrom api

能够对 tensor 中的数据进行复制

*/

//*********
   TensorValPrint_int8(*lNetInputTensor.get());
   //std::unique_ptr<Tensor> lNetInputTensor_cpy = nullptr;

    lNetInputTensor_cpy = std::unique_ptr<Tensor>(Tensor::Create<>(
      "NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
      TensorShape<TensorFormat_t::NHWC>{aBatch, lInputHeight, lInputWidth, lInputChannel}));

    lNetInputTensor_cpy->CopyDataFrom(*lNetInputTensor.get());

    TensorValPrint(*lNetInputTensor_cpy.get());

    printf(" 379 \n");
//*************
//*************

//end:
  return lStatus == Status_t::SUCCESS ? 0 : 1;
}
















/*


*/



/* depth wise 3x3 convolution */
static int concat_node_construction(int              aBatch,
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
             NodeFactory<ConcatConfig>::Create(
                ConcatConfig{0}),
              {lNetInputTensor.get(), lNetInputTensor.get()}, {outputTensor});
     
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







/* depth wise 3x3 convolution */
static int deconv2d_net_construction(int              aBatch,
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
             NodeFactory<DeConvConfig>::Create(
                DeConvConfig{{akH, akW}, {aStrideH, aStrideW}, {1, 1}, {akH, akW},std::move(aPadding), std::move(aActivation)}),
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








/* depth wise 3x3 convolution */
static int CrossCorr_net_construction(int              aBatch,
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
             NodeFactory<CrossCorrConfig>::Create(
                CrossCorrConfig{{akH, akW}, {aStrideH, aStrideW}, {1, 1}, std::move(aPadding), std::move(aActivation)}),
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

int conv_layer_test(int checkRef)
{
  int lRetVal = 0;

  std::cout << std::endl;
  std::cout << "////////////////////////////////////////////////////\n";
  std::cout << "//////////// conv_layer_test /////////////\n";
  std::cout << "////////////////////////////////////////////////////" << std::endl;

  /* aBatch, akW, akH, aWidth, aHeight, aStrideW, aStrideH, aInputChannels, aOutputChannels, aGroup, aPadding, aActivation,      checkRef              */
  /*  N          K         Input               Stride            IC              OC            group, pad,     activation,      checkResultWithCRef    */
  //1x1s1
  lRetVal += conv2d_net_construction(5, 1, 1, 1, 56, 1, 1, 120, 30, 3, {PaddingScheme_t::SAME, 0, 0, 0, 0}, {ActivationFunction_t::NONE, 0.0, 0.0}, checkRef); // Conv_kh1_kw1_sh1_sw1_ph0_pw0_g3_#out30_5_120_56_1
 
  lRetVal += depthconv_net_construction(10, 3, 3, 28, 28, 1, 1, 30, {PaddingScheme_t::SAME, 0, 0, 0, 0}, {ActivationFunction_t::NONE, 0.0, 0.0}, checkRef); // DepthConv_kh3_kw3_sh1_sw1_ph1_pw1_g30_#out30_10_30_28_28
 
  lRetVal += deconv2d_net_construction(10, 3, 3, 28, 28, 1, 1, 30, {PaddingScheme_t::SAME, 0, 0, 0, 0}, {ActivationFunction_t::NONE, 0.0, 0.0}, checkRef); // DepthConv_kh3_kw3_sh1_sw1_ph1_pw1_g30_#out30_10_30_28_28
 
  //lRetVal += CrossCorr_net_construction(10, 3, 3, 28, 28, 1, 1, 30, {PaddingScheme_t::SAME, 0, 0, 0, 0}, {ActivationFunction_t::NONE, 0.0, 0.0}, checkRef); // DepthConv_kh3_kw3_sh1_sw1_ph1_pw1_g30_#out30_10_30_28_28
 
  lRetVal += concat_node_construction(10, 3, 3, 28, 28, 1, 1, 30, {PaddingScheme_t::SAME, 0, 0, 0, 0}, {ActivationFunction_t::NONE, 0.0, 0.0}, checkRef); // DepthConv_kh3_kw3_sh1_sw1_ph1_pw1_g30_#out30_10_30_28_28
 


printf("1328 \n");
     



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

#endif 
