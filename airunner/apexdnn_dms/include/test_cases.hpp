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
#ifndef TEST_CASES_HPP__
#define TEST_CASES_HPP__

#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <stdio.h>
#include <iomanip>

#include <airunner_public_importer.hpp>
#include <airunner_public.hpp>
#include <airunner_public_apex.hpp>
#include <airunner_postprocessing.hpp>
#include <preprocessing.h>
#include <oal.h>
#include "read_image.hpp"
#include "display_image.hpp"

int case_mobilenet(const std::string& aMnetGraph, const std::string& aResultGraph, const std::string& aImageFile, const std::string& aSlimLabelsFile);
int case_mssd(const std::string& aMssdGraph, const std::string& aImageFile, const std::string& aLabel, int numClasses = 91, int anchorGenVer = 1);
int mssd_apex_loop(const std::string& aMssdGraph, const std::string& aDescriptionFile, const std::string& aLabel, int numClasses = 91, int anchorGenVer = 1);
int mobilenet_loop(const std::string& aMnetGraph, const std::string& aResultGraph, const std::string& aDescriptionFile, const std::string& aSlimLabelsFile);

#endif /* TEST_CASES_HPP__ */
