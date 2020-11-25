##############################################################################
#
# Freescale Confidential Proprietary
#
# Copyright (c) 2016 Freescale Semiconductor;
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

SDK_ROOT := $(call path_relative_to,$(CURDIR),$(CURR_SDK_ROOT))
VPATH ?=  $(SDK_ROOT)/demos/airunner/utils/src

ISP_GRAPH = mipi_simple
SEQ_MODE = dynamic
SEQ_FW = $(SEQ_MODE)_$(ISP_GRAPH)

##############################################################################
# ARM_APP
##############################################################################

ARM_APP = recognize_ncnn

ARM_DEFS += -DVSDK_UMAT_USE_OPENCV


VPATH += ../src/align
VPATH += ../src/common
VPATH += ../src/detect/centerface
VPATH += ../src/detect/mtcnn
VPATH += ../src/detect/retinaface
VPATH += ../src/detect
VPATH += ../src/landmark
VPATH += ../src/landmark/zqlandmark
VPATH += ../src/recognize/mobilefacenet
VPATH += ../src/recognize



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
	main.cpp							                                        \
	face_engine.cpp					                                                \





# ARM_APP_SRCS +=                                                               \
#     src/*.cpp            \
#     src/align/*.cpp      \
#     src/common/*.cpp   \
#     src/detect/*.cpp   \
#     src/detect/retinaface/*.cpp   \
#     src/detect/mtcnn/*.cpp  \
#     src/detect/centerface/*.cpp   \
#     src/landmark/*.cpp  \
#     src/landmark/zqlandmark/*.cpp   \
#     src/recognize/*.cpp   \
#     src/recognize/mobilefacenet/*.cpp   \


ARM_INCS +=                                                                  \
    -I../src							                                        \
    -I../include							                                        \
    -I../src/align						                                        \
    -I../src/common				                                    \
    -I../src/detect/centerface					                                    \
    -I../src/detect/mtcnn					                                \
    -I../src/detect/retinaface					                                \
    -I../src/detect				                                        \
    -I../src/landmark				                                    \
    -I../src/recognize				                                                \
    -I../src/recognize/mobilefacenet					                    \


ARM_INCS +=                                                                  \
    -I$(SDK_ROOT)/platform/s32_v234                                          \
    -I$(SDK_ROOT)/3rdparty/ocv/linux-arm64/include                   \
    -I$(SDK_ROOT)/include                                                    \
	-I$(SDK_ROOT)/libs/utils/umat/include                                    \
	-I$(SDK_ROOT)/libs/utils/oal/user/include                                \
	-I$(SDK_ROOT)/libs/utils/oal/kernel/include                              \
    -I$(SDK_ROOT)/libs/utils/common/include                                  \
    -I$(SDK_ROOT)/isp/inc                                                    \
    -I$(SDK_ROOT)/libs/io/frame_io/include                                   \
    -I$(SDK_ROOT)/3rdparty/ncnn/include/ncnn   \
    -I$(SDK_ROOT)/isp/graphs/$(ISP_GRAPH)/inc                                \
    -I$(SDK_ROOT)/libs/io/dcu/include                            \
    -I/media/jcq/Soft/Dlib/dlib-master/Install_arm/include         \
    -I.                             \





##############################################################################
# STANDALONE SPECIFIC INCLUDES
##############################################################################	
ifneq (,$(findstring -sa,$(ODIR)))

ARM_APP_LIBS +=                                                              \
    $(SDK_ROOT)/libs/startup/v234ce_standalone/$(ODIR)/libv234ce.a           \
    $(SDK_ROOT)/libs/io/i2c/$(ODIR)/libi2c.a                                 \
    $(SDK_ROOT)/libs/io/uartlinflex_io/$(ODIR)/liblinflex.a        \
    $(SDK_ROOT)/3rdparty/ncnn/lib/libncnn.a          \              



ARM_LDOPTS +=                                                                \
    -lopencv_core                                                            \
    -lopencv_imgproc                                                         \
    -lopencv_objdetect                                                       \
    -lopencv_highgui                                                         \
    -lopencv_ml                                                              \
    -lopencv_imgcodecs                                                       \
    -lopencv_flann                                                           \
    -lopencv_highgui                                                         \
    -lopencv_calib3d                                                         \
    -lopencv_core                                                            \
    -lopencv_imgcodecs                                                       \
    -lopencv_imgproc                                                         \
    -lopencv_videoio                                                         \
    -lavcodec                                                                \
    -lavformat                                                               \
    -lavutil                                                                 \
    -lswresample                                                             \
    -lswscale                                                                \
    -L$(SDK_ROOT)/3rdparty/boost_1_62_0/stage/lib                            \
    -lboost_system                                                           \
    -lboost_thread                                                           \
    -lopencv_highgui                                                       \
    -lopencv_objdetect 



endif


ARM_INCS +=                                                                  \
    -I$(SDK_ROOT)/platform/s32_v234                                          \
    -I$(OPENCV_ROOT)/include                                                 \
    -I$(SDK_ROOT)/include                                                    \
	-I$(SDK_ROOT)/libs/utils/umat/include                                    \
	-I$(SDK_ROOT)/libs/utils/oal/user/include                                \
	-I$(SDK_ROOT)/libs/utils/oal/kernel/include                              \
    -I$(SDK_ROOT)/libs/utils/common/include                                  \
    -I$(SDK_ROOT)/isp/inc                                                    \
    -I$(SDK_ROOT)/libs/io/frame_io/include                                   \
    -I$(SDK_ROOT)/isp/graphs/$(ISP_GRAPH)/inc                                \
    -I$(SDK_ROOT)/libs/io/dcu/include                            \
    -I/media/jcq/Soft/Dlib/dlib-master/Install_arm/include    \
    -I../include                                                                         \
    -I$(SDK_ROOT)/demos/airunner/utils/include                                           \
    -I$(SDK_ROOT)/3rdparty/protobuf/include                                              \
    -I$(SDK_ROOT)/libs/apex/icp/include                                                  \
    -I$(SDK_ROOT)/libs/apex/acf/include                                                  \
    -I$(SDK_ROOT)/libs/io/frame_io/include                                               \
    -I$(SDK_ROOT)/libs/io/dcu/include                                                    \
    -I$(SDK_ROOT)/libs/apex/drivers/user/include                                         \
    -I$(SDK_ROOT)/libs/dnn/airunner/preprocessing/include                                \
 


ARM_APP_LIBS +=                                                              \
    $(SDK_ROOT)/libs/io/frame_io/$(ODIR)/libframe_io.a                       \
    $(SDK_ROOT)/libs/io/sdi/$(ODIR)/libsdi.a                                 \
    $(SDK_ROOT)/libs/isp/csi/user/$(ODIR)/libcsidrv.a                        \
    $(SDK_ROOT)/libs/isp/cam_generic/user/$(ODIR)/libcamdrv.a                \
    $(SDK_ROOT)/libs/utils/log/$(ODIR)/liblog.a                              \
	$(SDK_ROOT)/libs/utils/common/$(ODIR)/libcommon.a                        \
    $(SDK_ROOT)/libs/utils/umat/$(ODIR)/libumat.a                            \
    $(SDK_ROOT)/libs/isp/sequencer/user/$(ODIR)/libseqdrv.a                  \
    $(SDK_ROOT)/libs/isp/fdma/user/$(ODIR)/libfdmadrv.a                      \
    $(SDK_ROOT)/isp/graphs/$(ISP_GRAPH)/$(SEQ_FW)/$(ODIR)/lib$(SEQ_FW).a     \
    /media/jcq/Soft/Dlib/dlib-master/Install_arm/lib/libdlib.a          \
    $(SDK_ROOT)/3rdparty/ncnn/lib/libncnn.a          \
    $(SDK_ROOT)/libs/apexcv_base/apexcv_core/$(ODIR)/apexcv_core.a                       \
    $(SDK_ROOT)/libs/dnn/airunner/core/$(ODIR)/airunner.a                                \
    $(SDK_ROOT)/libs/dnn/airunner/importer/$(ODIR)/airunner_importer.a                   \
    $(SDK_ROOT)/libs/dnn/airunner/nodes/apu2/$(ODIR)/airunner_apu2_nodes.a               \
    $(SDK_ROOT)/libs/dnn/airunner/preprocessing/$(ODIR)/airunner_preprocessing.a         \
    $(SDK_ROOT)/libs/dnn/airunner/postprocessing/$(ODIR)/airunner_postprocessing.a       \
    $(SDK_ROOT)/libs/apex/acf/$(ODIR)/libacf.a                                           \
    $(SDK_ROOT)/libs/apex/icp/$(ODIR)/libicp.a                                           \
    $(SDK_ROOT)/libs/io/frame_io/$(ODIR)/libframe_io.a                                   \
    $(SDK_ROOT)/libs/utils/communications/$(ODIR)/lib_communications.a                   \
    $(SDK_ROOT)/libs/apex/drivers/user/$(ODIR)/libapexdrv.a                              \
    $(SDK_ROOT)/libs/utils/log/$(ODIR)/liblog.a             \
    #$(SDK_ROOT)/libs/utils/oal/user/$(ODIR)/liboal.a                         \



ARM_LDOPTS +=                                                                \
    -lopencv_core                                                            \
    -lopencv_imgproc                                                         \
    -lopencv_objdetect                                                       \
    -lopencv_highgui                                                         \
    -lopencv_ml                                                              \
    -lopencv_imgcodecs                                                       \
    -lopencv_flann                                                           \
    -lopencv_highgui                                                         \
    -lopencv_calib3d                                                         \
    -lopencv_core                                                            \
    -lopencv_imgcodecs                                                       \
    -lopencv_imgproc                                                         \
    -lopencv_videoio                                                         \
    -lavcodec                                                                \
    -lavformat                                                               \
    -lavutil                                                                 \
    -lswresample                                                             \
    -lswscale                                                                \
    -L$(SDK_ROOT)/3rdparty/boost_1_62_0/stage/lib                            \
    -lboost_system                                                           \
    -lboost_thread                                                           \
    -lopencv_highgui                                                       \
    -lopencv_objdetect 

##############################################################################
# STANDALONE SPECIFIC INCLUDES
##############################################################################
ifneq (,$(findstring -sa,$(ODIR)))

ARM_APP_LIBS +=                                                              \
    $(SDK_ROOT)/libs/startup/v234ce_standalone/$(ODIR)/libv234ce.a           \
    $(SDK_ROOT)/libs/io/i2c/$(ODIR)/libi2c.a                                 \
    $(SDK_ROOT)/libs/io/semihost/$(ODIR)/libSemihost.a                       \
    $(SDK_ROOT)/libs/io/uartlinflex_io/$(ODIR)/liblinflex.a                  \
    $(SDK_ROOT)/libs/io/dcu/$(ODIR)/libdcu.a            

ARM_LDOPTS +=                                                                \
    -lzlib                 
    
##############################################################################
# LINUX SPECIFIC INCLUDES
##############################################################################	    
else

ARM_APP_LIBS +=                                                              \
    $(SDK_ROOT)/libs/isp/jpegdec/user/$(ODIR)/libjpegdecdrv.a                \
    $(SDK_ROOT)/libs/isp/h264enc/user/$(ODIR)/libh264encdrv.a                \
    $(SDK_ROOT)/libs/isp/h264dec/user/$(ODIR)/libh264decdrv.a                \
    $(SDK_ROOT)/libs/isp/viu/user/$(ODIR)/libviudrv.a                        \
    
endif

ifneq (,$(findstring -integrity,$(ODIR)))

ARM_LDOPTS +=                                                                \
    -L$(SDK_ROOT)/ocv/integrity-arm/share/OpenCV/3rdparty/lib                \
    -lopencv_core                                                            \
    -lposix                                                                  \
    -livfs                                                                   \
    -lIlmImf                                                                 \
    -lzlib                                                                   \
    --exceptions

endif
