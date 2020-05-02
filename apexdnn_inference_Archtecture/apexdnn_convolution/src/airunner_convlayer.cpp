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
#include "apex.h"
#include <airunner_public.hpp>
#include <airunner_node_config.hpp>
#include <airunner_public_apex.hpp>
#include "airunner_convlayer.hpp"
#include "tensor_util.hpp"


#define Convolution true


using namespace airunner;


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
                                    const PaddingConfig&    aPadding,
                                    const ActivationConfig& aActivation,
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
  if(aActivation.mType == ActivationFunction::NONE)
    printf("Testing aActivation.mType: NONE\n");
  if(aActivation.mType == ActivationFunction::RELU)
    printf("Testing aActivation.mType: RELU\n");
  if(aActivation.mType == ActivationFunction::BRELU)
    printf("Testing aActivation.mType: BRELU\n");

  // Create Workspace
  TargetInfo               lRefInfo  = TargetInfo();
  ApexTargetInfo lApexInfo = ApexTargetInfo();
  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::map<std::string, TargetInfo*>{
      {target::REF, &lRefInfo}, {target::APEX, &lApexInfo}}));

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
  lNetInputTensor->Allocate(Allocation_t::OAL);
  lNetInputTensor->SetQuantParams({QuantInfo(-6, 6)});
  internal::RandomizeTensor(*lNetInputTensor.get(), 0);

  auto lCpuNetInputTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
      "NET_INPUT_TENSOR_COPY", DataType_t::SIGNED_8BIT,
      TensorShape<TensorFormat_t::NHWC>{aBatch, lInputHeight, lInputWidth, lInputChannel}));
  lCpuNetInputTensor->Allocate(Allocation_t::HEAP);
  lCpuNetInputTensor->SetQuantParams({QuantInfo(-6, 6)});
  lCpuNetInputTensor->CopyDataFrom(*lNetInputTensor.get());

  // Create Weight tensor, fill with random data
  auto lWeightTensor = std::unique_ptr<Tensor>(
      Tensor::Create<>("WEIGHT_TENSOR", DataType_t::SIGNED_8BIT,
                       TensorShape<TensorFormat_t::OIHW>{lOutputChannel/aGroup, lInputChannel, akH, akW}));
  lWeightTensor->Allocate(Allocation_t::HEAP);
  internal::RandomizeTensor(*lWeightTensor.get(), 0);
  lWeightTensor->SetQuantParams({QuantInfo(-0.3, 0.8)});

  // Create bias tensor, fill with random data
  auto lBiasTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
      "BIAS_TENSOR", DataType_t::SIGNED_32BIT,
      TensorShape<TensorFormat_t::OIHW>{lOutputChannel, 1, 1, 1}));
  lBiasTensor->Allocate(Allocation_t::HEAP);
  internal::RandomizeTensor(*lBiasTensor.get(), 0);
  lBiasTensor->SetQuantParams({QuantInfo(-0.3, 0.8)});

  // Build up the graph
  auto lGraph = std::unique_ptr<Graph>(new Graph(lWorkspace.get()));

  // Create output tensor
  auto outputTensor = lGraph->AddTensor("NET_OUTPUT_TENSOR");
  outputTensor->SetQuantParams({QuantInfo(-106, 106)});

  if (aGroup == 1)
  {
    lGraph->AddNode(
      CreateConvNode(ConvConfig{{akH, akW}, {aStrideH, aStrideW}, {1, 1}, {aPadding}, {aActivation}}),
      {lNetInputTensor.get(), lWeightTensor.get(), lBiasTensor.get()}, {outputTensor});
  }
  else
  {
    lGraph->AddNode(
      CreateGroupConvNode(ConvConfig{{akH, akW}, {aStrideH, aStrideW}, {1, 1}, {aPadding}, {aActivation}}, aGroup),
      {lNetInputTensor.get(), lWeightTensor.get(), lBiasTensor.get()}, {outputTensor});
  }

  // Run APEX
  lStatus = lGraph->SetTargetHint(target::APEX);
  if(Status_t::SUCCESS != lStatus)
  {
    std::cout << "Set APEX target failed" << std::endl;
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



/* 该 run()就是在 apex 中运行卷积的过程，需要进行 convolution 的运算，返回值为status!
   把卷积操作放到这部分。20200422

#1.如何实现convolution?

#2.

*/
#if Convolution


 compute_conv(fm,kernel):
    [h,w]=fm.shape
    [k,_]=kernel.shape 
    r=int(k/2)
    #定义边界填充0后的map
    padding_fm=np.zeros([h+2,w+2],np.float32)
    #保存计算结果
    rs=np.zeros([h,w],np.float32)
    #将输入在指定该区域赋值，即除了4个边界后，剩下的区域
    padding_fm[1:h+1,1:w+1]=fm 
    #对每个点为中心的区域遍历
    for i in range(1,h+1):
        for j in range(1,w+1): 
            #取出当前点为中心的k*k区域
            roi=padding_fm[i-r:i+r+1,j-r:j+r+1]
            #计算当前点的卷积,对k*k个点点乘后求和
            rs[i-1][j-1]=np.sum(roi*kernel)
 
    return rs

  compute_conv(fm,kernel){
    
  }


 my_conv2d(input,weights):
    [c,h,w]=input.shape
    [_,k,_]=weights.shape
    outputs=np.zeros([h,w],np.float32)
 
    #对每个feature map遍历，从而对每个feature map进行卷积
    for i in range(c):
        #feature map==>[h,w]
        f_map=input[i]
        #kernel ==>[k,k]
        w=weights[i]
        rs =compute_conv(f_map,w)
        outputs=outputs+rs   

      return outputs





#endif





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

    lApexNetOutput->CopyDataFrom(*outputTensor);
    memset(outputTensor->DataPtr(), 0, outputTensor->Size());

    lStatus = lGraph->SetTargetHint(target::REF);
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

    if(internal::TensorEqual(*lApexNetOutput.get(), *outputTensor))
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
static int depthconv_net_construction( int                     aBatch,
                                       int                     akW,
                                       int                     akH,
                                       int                     aWidth,
                                       int                     aHeight,
                                       int                     aStrideW,
                                       int                     aStrideH,
                                       int                     aInputChannels,
                                       const PaddingConfig&    aPadding,
                                       const ActivationConfig& aActivation,
                                       int                     aFastProf)
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
  if(aActivation.mType == ActivationFunction::NONE)
    printf("Testing aActivation.mType: NONE\n");
  if(aActivation.mType == ActivationFunction::RELU)
    printf("Testing aActivation.mType: RELU\n");
  if(aActivation.mType == ActivationFunction::BRELU)
    printf("Testing aActivation.mType: BRELU\n");

  // Create Workspace
  TargetInfo lRefInfo = TargetInfo();
  ApexTargetInfo lApexInfo = ApexTargetInfo();
  auto lWorkspace = std::unique_ptr<Workspace>(new Workspace(std::map<std::string, TargetInfo *>{
      {target::REF, &lRefInfo},
      {target::APEX, &lApexInfo}}));
  
  {
      // Create Input tensor, fill with random data
      auto lNetInputTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
          "NET_INPUT_TENSOR", DataType_t::SIGNED_8BIT,
          TensorShape<TensorFormat_t::NHWCE0>{aBatch, lInputHeight, lInputWidth, lInputChannel},
          TensorLayout<TensorFormat_t::NHWCE0>(TensorDim_t::WIDTH, 2)));
      lNetInputTensor->Allocate(Allocation_t::OAL);
      internal::RandomizeTensor(*lNetInputTensor.get(), 0);
      lNetInputTensor->SetQuantParams({QuantInfo(-6, 6)});

      // Create Weight tensor, fill with random data
      auto lWeightTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
          "WEIGHT_TENSOR", DataType_t::SIGNED_8BIT,
          TensorShape<TensorFormat_t::OIHW>{1, lInputChannel, lFilterHeight, lFilterWidth}));
      lWeightTensor->Allocate(Allocation_t::HEAP);
      internal::RandomizeTensor(*lWeightTensor.get(), 0);
      lWeightTensor->SetQuantParams({QuantInfo(-0.3, 0.8)});

      // Create bias tensor, fill with random data
      auto lBiasTensor = std::unique_ptr<Tensor>(Tensor::Create<>(
          "BIAS_TENSOR", DataType_t::SIGNED_32BIT,
          TensorShape<TensorFormat_t::OIHW>{lOutputChannel, 1, 1, 1}));
      lBiasTensor->Allocate(Allocation_t::HEAP);
      internal::RandomizeTensor(*lBiasTensor.get(), 0);
      lBiasTensor->SetQuantParams({QuantInfo(-0.3, 0.8)});

      // Build up the graph
      auto lGraph = std::unique_ptr<Graph>(new Graph(lWorkspace.get()));

      // Create output tensor
      auto outputTensor = lGraph->AddTensor("NET_OUTPUT_TENSOR");
      outputTensor->SetQuantParams({QuantInfo(0, 8.0)});

      lGraph->AddNode(
             CreateDepthWiseConvNode(
                ConvConfig{{akH, akW}, {aStrideH, aStrideW}, {1, 1}, aPadding, {aActivation}}),
              {lNetInputTensor.get(), lWeightTensor.get(), lBiasTensor.get()}, {outputTensor});
     
      // Run APEX
      lStatus = lGraph->SetTargetHint(target::APEX);
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

        internal::CopyTensor(*outputTensor, lApexNetOutput);
        memset(outputTensor->DataPtr(), 0, outputTensor->Size());

        lStatus = lGraph->SetTargetHint(target::REF);
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

        if(internal::TensorEqual(lApexNetOutput, *outputTensor))
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
  lRetVal += conv2d_net_construction(5, 1, 1, 1, 56, 1, 1, 120, 30, 3, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh1_kw1_sh1_sw1_ph0_pw0_g3_#out30_5_120_56_1
  
  //3x1s1
  lRetVal += conv2d_net_construction(1, 3, 1, 32, 18, 1, 1, 320, 160, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh1_kw3_sh1_sw1_ph0_pw1_g1_#out160_1_320_18_32
  
  //3x1s2
  lRetVal += conv2d_net_construction(1, 3, 1, 256, 144, 2, 2, 16, 64, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh1_kw3_sh2_sw2_ph0_pw1_g1_#out64_1_16_144_256

  //1x3s1
  lRetVal += conv2d_net_construction(30, 1, 3, 3, 72, 1, 1, 64, 128, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh3_kw1_sh1_sw1_ph1_pw0_g1_#out128_30_64_72_3
  
  //1x3s2
  lRetVal += conv2d_net_construction(30, 1, 3, 1, 72, 1, 2, 64, 64, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh3_kw1_sh2_sw1_ph1_pw0_g1_#out64_30_64_72_1
 
  //1x3s2
  lRetVal += conv2d_net_construction(5, 1, 3, 1, 56, 2, 2, 30, 30, 3, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh3_kw1_sh2_sw2_ph1_pw0_g3_#out30_5_30_56_1

  //1x5s1
  lRetVal += conv2d_net_construction(30, 1, 5, 1, 72, 1, 1, 256, 128, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh5_kw1_sh1_sw1_ph2_pw0_g1_#out128_30_256_72_1
  
  //3x5s1
  lRetVal += conv2d_net_construction(30, 3, 5, 9, 72, 1, 1, 64, 128, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh5_kw3_sh1_sw1_ph2_pw1_g1_#out128_30_64_72_9

  //5x3s1
  lRetVal += conv2d_net_construction(1, 5, 3, 1024, 576, 2, 2, 3, 64, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh3_kw5_sh2_sw2_ph1_pw2_g1_#out64_1_3_576_1024

  //gemm2d
  lRetVal += conv2d_net_construction(30, 18, 1, 18, 72, 1, 1, 16, 12, 2, {PaddingScheme::VALID}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh1_kw18_sh1_sw1_ph0_pw0_g2_#out12_30_16_72_18
  
  //1x18s1
  lRetVal += conv2d_net_construction(30, 1, 18, 18, 72, 1, 1, 16, 16, 4, {PaddingScheme::VALID}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh18_kw1_sh1_sw1_ph0_pw0_g4_#out16_30_16_72_18

  //3x3s2
  lRetVal += conv2d_net_construction(15, 3, 3, 224, 224, 2, 2, 3, 12, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh3_kw3_sh2_sw2_ph1_pw1_g1_#out12_15_3_224_224
  
  //3x3s1
  lRetVal += conv2d_net_construction(2, 3, 3, 105, 57, 1, 1, 32, 96, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh3_kw3_sh1_sw1_ph1_pw1_g1_#out96_2_32_57_105
 
  //3x3s2 grayscale
  lRetVal += conv2d_net_construction(1, 3, 3, 512, 224, 2, 2, 1, 12, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh3_kw3_sh2_sw2_ph1_pw1_g1_#out12_1_1_224_512
  
  //3x3s2 RGB
  lRetVal += conv2d_net_construction(10, 3, 3, 224, 224, 2, 2, 3, 8,  1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh3_kw3_sh2_sw2_ph1_pw1_g1_#out8_10_3_224_224
 
  //7x7s2 RGB   
  lRetVal += conv2d_net_construction(1, 7, 7, 800, 448, 2, 2, 3, 32, 1, {PaddingScheme::VALID}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh7_kw7_sh2_sw2_ph3_pw3_g1_#out32_1_3_448_800

  //7x7s2 grayscale  
  lRetVal += conv2d_net_construction(1, 7, 7, 512, 288, 2, 2, 1, 64, 1, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // Conv_kh7_kw7_sh2_sw2_ph3_pw3_g1_#out64_1_1_288_512

  //depthwise 3x3s2
  lRetVal += depthconv_net_construction(10, 3, 3, 56, 56, 2, 2, 30, {PaddingScheme::SAME}, {ActivationFunction::RELU, 0}, checkRef); // DepthConv_kh3_kw3_sh2_sw2_ph1_pw1_g30_#out30_10_30_56_56
  
  //depthwise 3x3s1
  lRetVal += depthconv_net_construction(10, 3, 3, 28, 28, 1, 1, 30, {PaddingScheme::SAME}, {ActivationFunction::NONE, 0}, checkRef); // DepthConv_kh3_kw3_sh1_sw1_ph1_pw1_g30_#out30_10_30_28_28





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
