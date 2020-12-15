##############################################################################
#
# Freescale Confidential Proprietary
#
# Copyright (c) 2015 Freescale Semiconductor;
# All Rights Reserved
#
##############################################################################
#
# THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
##############################################################################

# APP_FUNC ?= DMS_AR0144_MAX9286_MIPI
APP_FUNC ?= DMS_OV9284_MAX9286_MIPI
# APP_FUNC ?= DMS_VIDEO_INPUT

SDK_ROOT := $(call path_relative_to,$(CURDIR),$(CURR_SDK_ROOT))

# ARM_APP   = $(APP_FUNC)
#ISP_GRAPH = ar0144_max9286_mipi
#APP_VERSION: 
#			0--DMS_AR0144_MAX9286_MIPI
#           1--DMS_OV9284_MAX9286_MIPI
#           2--DMS_VIDEO_INPUT

ifeq ($(APP_FUNC),DMS_AR0144_MAX9286_MIPI)
    APP_VERSION = 0
    SEQ_MODE  = dynamic
    ISP_GRAPH = ar0144_max9286_mipi
    ARM_APP   = dms_ar0144_max9286_mipi
endif
ifeq ($(APP_FUNC),DMS_OV9284_MAX9286_MIPI)
    APP_VERSION = 1
    SEQ_MODE  = dynamic
    ISP_GRAPH = ov9284_ub964_mipi
    ARM_APP   = dms_ov9284_max9286_mipi
endif
ifeq ($(APP_FUNC),DMS_VIDEO_INPUT)
    APP_VERSION = 2
    ARM_APP   = dms_video_input
endif

#SEQ_MODE = dynamic
SEQ_FW = $(SEQ_MODE)_$(ISP_GRAPH)
##############################################################################
# isp_app
##############################################################################

VPATH += ../src/ImageGrabber
VPATH += ../src/Utils
VPATH += ../src/Display
VPATH += ../src/Render
VPATH += ../src/Render/Font
VPATH += ../src/Render/Logo
VPATH += ../src/Render/Colorbar
VPATH += ../src/Render/HeadModel
VPATH += ../src/Render/Shaders
VPATH += ../src/Render/Settings


# FACE RECOGNIZE *******

VPATH += ../src/FaceRecog/align
VPATH += ../src/FaceRecog/common
VPATH += ../src/FaceRecog/detect/centerface
VPATH += ../src/FaceRecog/detect/mtcnn
VPATH += ../src/FaceRecog/detect/retinaface
VPATH += ../src/FaceRecog/detect
VPATH += ../src/FaceRecog/landmark
VPATH += ../src/FaceRecog/landmark/zqlandmark
VPATH += ../src/FaceRecog/recognize/mobilefacenet
VPATH += ../src/FaceRecog/recognize

# ****************FACE RECOGNIZE 




ARM_DEFS += -DVSDK_UMAT_USE_OPENCV                                                  \
			-DAPP_VERSION=$(APP_VERSION)                                            \

ARM_DEFS += -DLINUX -DEGL_API_FB -DGPU_TYPE_VIV -DGL_GLEXT_PROTOTYPES 

ARM_APP_SRCS +=                                                                     \
	main.cpp															 

#Render source files
ARM_APP_SRCS +=                                                                     \
	FontRenderer.cpp														        \
    LogoRenderer.cpp														        \
    ColorbarRenderer.cpp                                                            \
    HeadRenderer.cpp      				                                            \
	gl_shaders.cpp															        \
	XmlSettings.cpp															        \
	Render.cpp		

#Utils source files
ARM_APP_SRCS +=                                                                     \
	KeyConnect.cpp							                                        \
	XMLReader.cpp					                                                \



# FACE RECOGNIZE *******


#Render source files
ARM_APP_SRCS +=                                                                     \
	aligner.cpp														        \
    common.cpp														        \
    centerface.cpp                                                            \
    mtcnn.cpp      				                                            \
    landmarker.cpp    \
    mobilefacenet.cpp                                                            \
    recognizer.cpp                                                          \
	retinaface.cpp															        \
	detector.cpp															        \
	zq_landmarker.cpp		

#Utils source files
ARM_APP_SRCS +=                                                                     \
	face_engine.cpp					                                                \


# **************************FACE RECOGNIZE *******









#ImageGrabber source files
ifneq (,$(filter $(APP_FUNC),DMS_AR0144_MAX9286_MIPI DMS_OV9284_MAX9286_MIPI))
ARM_APP_SRCS +=                                                                     \
	ImageGrabber.cpp			                                                
endif				                                                

#Display source files
ARM_APP_SRCS +=                                                                     \
    Display.cpp					                                                    \

#Common header files
ARM_INCS +=                                                                         \
    -I$(SDK_ROOT)/platform/s32_v234                                                 \
    -I$(OPENCV_ROOT)/include                                                        \
    -I$(SDK_ROOT)/include                                                           \
    -I$(SDK_ROOT)/libs/apex/acf/include                                             \
    -I$(SDK_ROOT)/libs/apex/common/include                                          \
    -I$(SDK_ROOT)/libs/apex/icp/include                                             \
    -I$(SDK_ROOT)/libs/apex/drivers/user/include                                    \
    -I$(SDK_ROOT)/libs/utils/common/include                                         \
    -I$(SDK_ROOT)/libs/utils/communications/include                                 \
    -I$(SDK_ROOT)/libs/utils/communications/src                                     \
    -I$(SDK_ROOT)/libs/utils/umat/include				                            \
    -I$(SDK_ROOT)/libs/io/sdi/include                                               \
    -I$(SDK_ROOT)/libs/io/framebuffer/user/include/linux                            \
    -I$(SDK_ROOT)/libs/io/image_camera_io/include                                   \
    -I$(SDK_ROOT)/libs/io/frame_io/include                                          \
    -I$(SDK_ROOT)/libs/io/dcu/include                                               \
    -I$(SDK_ROOT)/isp/inc                                                           \
    -I$(SDK_ROOT)/libs/isp/sequencer/kernel/include                                 \
    -I$(SDK_ROOT)/libs/apexcv_pro/gftt_corners/graphs/build-apu-$(APU_COMP)-sa-d    \
	-I$(SDK_ROOT)/libs/apexcv_pro/fast/graphs/build-apu-$(APU_COMP)-sa-d            \
	-I$(SDK_ROOT)/libs/apexcv_pro/lkpyramid/graphs/build-apu-$(APU_COMP)-sa-d       \
	-I$(SDK_ROOT)/libs/apexcv_pro/resize/graphs/build-apu-$(APU_COMP)-sa-d          \
    -I$(SDK_ROOT)/libs/apexcv_pro/image_pyramid/graphs/build-apu-$(APU_COMP)-sa-d   \
    -I$(SDK_ROOT)/libs/apexcv_pro/hog/graphs                                        \
    -I$(SDK_ROOT)/libs/apexcv_pro/hog/apu-$(APU_COMP)/$(ODIR)                                       \
    -I$(SDK_ROOT)/libs/apexcv_pro/hog/graphs/build-apu-$(APU_COMP)-sa-d             \
    -I$(SDK_ROOT)/libs/apexcv_ref/apexcv_pro/resize/include                         \
    -I$(SDK_ROOT)/libs/apexcv_ref/apexcv_pro/hog/include		                    \
    -I../include							                                        \
    -I../include/Render						                                        \
    -I../include/Render/Font					                                    \
    -I../include/Render/Logo					                                    \
    -I../include/Render/HeadModel					                                \
    -I../include/Render/Colorbar					                                \
    -I../include/Render/Shaders				                                        \
    -I../include/Render/Settings				                                    \
    -I../include/Utils				                                                \
    -I../include/Display					                                        \
    -I../include/DMS					                                            \
    -I../include/DMS/AIRunnerClassifier					                            \
    -I../include/DMS/CompressiveTracker                                             \
    -I../include/DMS/FaceDetectorDlib                                               \
    -I../include/DMS/FaceLandmarker                                                 \
    -I../include/DMS/HoGSVMDetector                                                 \
    -I../include/DMS/MatchedFilterDetector                                          \

#VIV lib header files
ARM_INCS +=                                                  \
    -I$(SDK_ROOT)/3rdparty/viv_lib/usr/include               \
    -I$(SDK_ROOT)/3rdparty/xml2/include/libxml2              \
	-I$(SDK_ROOT)/3rdparty/glm-0.9.8.5                       \
	-I$(SDK_ROOT)/3rdparty/assimp-4.0.0/build/include        \

#DMS header files
ARM_INCS +=                                                  \
    -I../include/DMS/HoGSVMDetector						     \
    -I../include/DMS/CompressiveTracker					     \
    -I../include/DMS/FaceLandmarker						     \
    -I../include/DMS/AIRunnerClassifier					     \
    -I../include/DMS/FaceDetectorDlib				     	 \
    -I../include/DMS/MatchedFilterDetector				     \
    -I../include/DMS/                                        \
    -I$(SDK_ROOT)/3rdparty/dlib-19.10

#AIRunner header files
ARM_INCS +=                                                               \
    -I$(SDK_ROOT)/demos/airunner/libs/utils/include                       \
    -I$(SDK_ROOT)/libs/dnn/airunner/preprocessing/include                 \
    -I$(SDK_ROOT)/libs/dnn/airunner/postprocessing/include                \
    -I$(SDK_ROOT)/3rdparty/iniparser4/src

#ImageGrabber source files
ifneq (,$(filter $(APP_FUNC),DMS_AR0144_MAX9286_MIPI DMS_OV9284_MAX9286_MIPI))
ARM_INCS +=                                                                         \
    -I$(SDK_ROOT)/isp/graphs/$(ISP_GRAPH)/inc                                       \
    -I../include/ImageGrabber
endif



# FACE RECOGNIZE *******


ARM_INCS +=                                                                  \
    -I../src							                                        \
    -I../include							                                        \
    -I../src/FaceRecog/align						                                        \
    -I../src/FaceRecog/common				                                    \
    -I../src/FaceRecog/detect/centerface					                                    \
    -I../src/FaceRecog/detect/mtcnn					                                \
    -I../src/FaceRecog/detect/retinaface					                                \
    -I../src/FaceRecog/detect				                                        \
    -I../src/FaceRecog/landmark				                                    \
    -I../src/FaceRecog/recognize				                                                \
    -I../src/FaceRecogrc/recognize/mobilefacenet					                    \
    -I$(SDK_ROOT)/3rdparty/ncnn/include/ncnn   \
#************************* FACE RECOGNIZE










#Common libs
ARM_APP_LIBS +=                                                                         \
    $(SDK_ROOT)/libs/io/frame_io/$(ODIR)/libframe_io.a                                  \
    $(SDK_ROOT)/libs/io/sdi/$(ODIR)/libsdi.a                                            \
    $(SDK_ROOT)/libs/io/dcu/$(ODIR)/libdcu.a                                            \
    $(SDK_ROOT)/libs/isp/cam_generic/user/$(ODIR)/libcamdrv.a                           \
    $(SDK_ROOT)/libs/isp/csi/user/$(ODIR)/libcsidrv.a                                   \
	$(SDK_ROOT)/libs/utils/communications/$(ODIR)/lib_communications.a                  \
    $(SDK_ROOT)/libs/utils/log/$(ODIR)/liblog.a                                         \
    $(SDK_ROOT)/libs/isp/sequencer/user/$(ODIR)/libseqdrv.a                             \
    $(SDK_ROOT)/libs/isp/fdma/user/$(ODIR)/libfdmadrv.a                                 \
    $(SDK_ROOT)/libs/utils/common/$(ODIR)/libcommon.a                                   \
    $(SDK_ROOT)/libs/utils/sumat/$(ODIR)/libsumat.a                                     \
    $(SDK_ROOT)/libs/utils/umat/$(ODIR)/libumat.a                                       \
    $(SDK_ROOT)/libs/apex/acf/$(ODIR)/libacf.a                                          \
    $(SDK_ROOT)/libs/apex/icp/$(ODIR)/libicp.a                                          \
    $(SDK_ROOT)/libs/apexcv_base/apexcv_common/$(ODIR)/apexcv_common.a                  \
    $(SDK_ROOT)/libs/apex/drivers/user/$(ODIR)/libapexdrv.a                             \
    $(SDK_ROOT)/libs/apexcv_base/apexcv_core/$(ODIR)/apexcv_core.a                      \
    $(SDK_ROOT)/libs/apexcv_base/integral_image/apu-$(APU_COMP)/$(ODIR)/apexcv_integral_image.a	        \
    $(SDK_ROOT)/libs/apexcv_pro/image_pyramid/apu-$(APU_COMP)/$(ODIR)/apexcv_pro_pyramid.a              \
    $(SDK_ROOT)/libs/apexcv_pro/hog/apu-$(APU_COMP)/$(ODIR)/apexcv_pro_hog.a            \
    $(SDK_ROOT)/libs/apexcv_ref/apexcv_pro/resize/$(ODIR)/resize_ref.a                  \
    $(SDK_ROOT)/libs/apexcv_ref/apexcv_pro/hog/$(ODIR)/hog_ref.a	                    \
    $(SDK_ROOT)/libs/apexcv_pro/resize/apu-$(APU_COMP)/$(ODIR)/apexcv_pro_resize.a      \
    $(SDK_ROOT)/libs/apexcv_pro/util/$(ODIR)/apexcv_pro_util.a

#DMS libs
ARM_APP_LIBS +=                                                                     \
    ../src/DMS/CompressiveTracker/$(ODIR)/libCompressiveTracker.a       		    \
    ../src/DMS/HoGSVMDetector/$(ODIR)/libHoGSVMDetector.a       			        \
    ../src/DMS/FaceLandmarker/$(ODIR)/libFaceLandmarker.a     			            \
    ../src/DMS/FaceDetectorDlib/$(ODIR)/libFaceDetectorDlib.a 			            \
    ../src/DMS/AIRunnerClassifier/$(ODIR)/libAIRunnerClassifier.a 			        \
    ../src/DMS/MatchedFilterDetector/$(ODIR)/libMatchedFilterDetector.a 			\
    ../src/DMS/$(ODIR)/libDMS.a        \
    $(SDK_ROOT)/3rdparty/ncnn/lib/libncnn.a      \

#AIRunner libs
ARM_APP_LIBS +=                                                                          \
    $(SDK_ROOT)/demos/airunner/libs/utils/$(ODIR)/demo_utils.a                           \
    $(SDK_ROOT)/3rdparty/iniparser4/$(ODIR)/libiniparser4.a   	                         \
    $(SDK_ROOT)/libs/dnn/airunner/core/$(ODIR)/airunner.a                                \
    $(SDK_ROOT)/libs/dnn/airunner/importer/$(ODIR)/airunner_importer.a                   \
    $(SDK_ROOT)/libs/dnn/airunner/nodes/cpu_opt/$(ODIR)/airunner_cpu_opt_nodes.a         \
    $(SDK_ROOT)/libs/dnn/airunner/nodes/apu2/$(ODIR)/airunner_apu2_nodes.a               \
    $(SDK_ROOT)/libs/dnn/airunner/preprocessing/$(ODIR)/airunner_preprocessing.a         \
    $(SDK_ROOT)/libs/dnn/airunner/postprocessing/$(ODIR)/airunner_postprocessing.a

#ImageGrabber source files
ifneq (,$(filter $(APP_FUNC),DMS_AR0144_MAX9286_MIPI DMS_OV9284_MAX9286_MIPI))
ARM_APP_LIBS +=                                                                     \
    $(SDK_ROOT)/isp/graphs/$(ISP_GRAPH)/$(SEQ_FW)/$(ODIR)/lib$(SEQ_FW).a
endif

ARM_LDOPTS +=                                                                \
	-L$(SDK_ROOT)/3rdparty/viv_lib/usr/lib		                             \
	-L$(SDK_ROOT)/3rdparty/xml2/lib		                                     \
	-L$(SDK_ROOT)/3rdparty/assimp-4.0.0/build/code		                     \
	-L$(SDK_ROOT)/3rdparty/ffmpeg/linux-arm64/lib	                         \
	-Wl,-rpath-link=$(SDK_ROOT)/3rdparty/viv_lib/usr/lib                     \
	-Wl,-rpath-link=$(SDK_ROOT)/3rdparty/xml2/lib                            \
	-Wl,-rpath-link=$(SDK_ROOT)/3rdparty/assimp-4.0.0/build/code             \
	-Wl,-rpath-link=$(SDK_ROOT)/3rdparty/ffmpeg/linux-arm64/lib           	 \
    -L$(SDK_ROOT)/3rdparty/tensorflow/$(EXTODIR)                             \
	  -l3rdparty_tensorflow                                                  \
    -L$(SDK_ROOT)/3rdparty/protobuf/$(EXTODIR)                               \
	  -lprotobuf-lite                                                        \
    -lopencv_core                                                            \
    -lopencv_imgcodecs                                                       \
    -lopencv_imgproc                                                         \
    -lopencv_videoio                                                         \
    -lopencv_video							                                 \
    -lopencv_calib3d							                             \
    -lopencv_flann							                                 \
    -lopencv_features2d							                             \
    -lopencv_highgui							                             \
    -lopencv_ml							     	                             \
    -lopencv_objdetect                                                       \
    -lavcodec                                                                \
    -lavformat                                                               \
    -lavutil                                                                 \
    -lswresample                                                             \
    -lswscale                                                                \
	-lGLESv2 -lEGL -lassimp -lxml2 											 \
          
    
ifneq (,$(findstring gnu-sa,$(ODIR))) 
  ARM_APP_LIBS +=                                                         \
    $(SDK_ROOT)/libs/io/i2c/$(ODIR)/libi2c.a                              \
    $(SDK_ROOT)/libs/io/dcu/$(ODIR)/libdcu.a                              \

endif

ifneq (,$(findstring gnu-linux,$(ODIR))) 
  ARM_APP_LIBS +=                                                         \
    $(SDK_ROOT)/libs/isp/jpegdec/user/$(ODIR)/libjpegdecdrv.a             \
    $(SDK_ROOT)/libs/isp/h264enc/user/$(ODIR)/libh264encdrv.a             \
    $(SDK_ROOT)/libs/isp/h264dec/user/$(ODIR)/libh264decdrv.a             \
    $(SDK_ROOT)/libs/isp/viu/user/$(ODIR)/libviudrv.a                     \

endif

##############################################################################
# STANDALONE SPECIFIC INCLUDES
##############################################################################	
ifneq (,$(findstring -sa,$(ODIR)))

ARM_APP_LIBS +=                                                              \
    $(SDK_ROOT)/libs/startup/v234ce_standalone/$(ODIR)/libv234ce.a           \
    $(SDK_ROOT)/libs/io/i2c/$(ODIR)/libi2c.a                                 \
    $(SDK_ROOT)/libs/io/semihost/$(ODIR)/libSemihost.a                       \
    $(SDK_ROOT)/libs/io/uartlinflex_io/$(ODIR)/liblinflex.a                         

ARM_LDOPTS +=                                                                \
    -lzlib 
    
endif
