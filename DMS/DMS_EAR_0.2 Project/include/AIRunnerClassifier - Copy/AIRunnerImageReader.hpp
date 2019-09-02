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

#ifndef READ_IMAGE_HPP__
#define READ_IMAGE_HPP__

#include <airunner_public.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>

#define RGB_TO_BGR_INDEX(i) (2-(i))
#define FLOAT_TO_INT8(f) ((float)(f-))


struct Normalize {
  float mInputMean;
  float mInputStd;
};

int ReadImageToTensorNorm(const std::string& aName, airunner::Tensor* aTensor, struct Normalize* aNormParams = nullptr, int n = 0);
int MatToTensor(cv::Mat tmpMat, airunner::Tensor* aTensor, struct Normalize* aNormParams = nullptr, int n = 0);

int ReadImageToTensor(const std::string& aName, airunner::Tensor* aTensor);
int TensorToImage(airunner::Tensor* aTensor, cv::Mat* aOutImage, int n = 0);
#endif /* READ_IMAGE_HPP__ */
