##############################################################################
#
# NXP Confidential Proprietary
#
# Copyright (c) 2018 NXP Semiconductor;
# All Rights Reserved
#
##############################################################################
#
# THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
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
# ISP_GRAPH = imx224_exp_ctrl
SEQ_MODE = dynamic
SEQ_FW = $(SEQ_MODE)_$(ISP_GRAPH)
##############################################################################
# ARM_APP
##############################################################################

ARM_APP = apex_mssd
ARM_DEFS += -DVSDK_UMAT_USE_OPENCV

ARM_APP_SRCS =                                                                           \
    main.cpp                                                                             \
    read_image.cpp                                                                       \
    display_image.cpp                                                                    \
    case_mobilenet.cpp                                                                   \
    mobilenet_loop.cpp                                                                   \
	  mssd_loop.cpp                                                                        \
	  case_mssd.cpp								\
	ai_app_task.cpp                   \
  ai_app_types.cpp                                                                        \
  ai_image_sensor_task.cpp								\
	ai_display_task.cpp								\
	ai_mssd_task.cpp									\
	ai_stdin_task.cpp									\
	
	

ARM_INCS +=                                                                              \
    -I../src                                                                 \
    -I../include                                                             \
    -I$(SDK_ROOT)/platform/s32_v234                                          \
    -I$(OPENCV_ROOT)/include                                                             \
    -I$(SDK_ROOT)/demos/airunner/utils/include                                           \
    -I$(SDK_ROOT)/3rdparty/protobuf/include                                              \
    -I$(SDK_ROOT)/include                                                                \
	-I$(SDK_ROOT)/libs/utils/umat/include                                    \
	-I$(SDK_ROOT)/libs/utils/oal/user/include                                \
	-I$(SDK_ROOT)/libs/utils/oal/kernel/include                              \
	-I$(SDK_ROOT)/libs/utils/communications/include                              \
	-I$(SDK_ROOT)/libs/utils/communications/src                              \
    -I$(SDK_ROOT)/isp/inc                                                    \
    -I$(SDK_ROOT)/libs/apex/icp/include                                                  \
    -I$(SDK_ROOT)/libs/apex/acf/include                                                  \
    -I$(SDK_ROOT)/libs/io/frame_io/include                                               \
    -I$(SDK_ROOT)/isp/graphs/$(ISP_GRAPH)/inc                                \
    -I$(SDK_ROOT)/libs/io/dcu/include                                                    \
    -I$(SDK_ROOT)/libs/apex/drivers/user/include                                         \
    -I$(SDK_ROOT)/libs/utils/oal/user/include                                            \
    -I$(SDK_ROOT)/libs/utils/common/include                                              \
    -I$(SDK_ROOT)/libs/apex/common/include                                   \
    -I$(SDK_ROOT)/3rdparty/iniparser4/src                                    \
    -I$(SDK_ROOT)/libs/io/frame_io/include                                   \
    -I$(SDK_ROOT)/libs/dnn/airunner/preprocessing/include                                \
    -I$(SDK_ROOT)/libs/dnn/airunner/postprocessing/include                               \
    -I$(SDK_ROOT)/3rdparty/boost_1_62_0                                      

ARM_APP_LIBS =                                                                           \
    $(SDK_ROOT)/libs/apexcv_base/apexcv_core/$(ODIR)/apexcv_core.a                       \
    $(SDK_ROOT)/3rdparty/protobuf/arm64/lib/libprotobuf-lite.a                           \
    $(SDK_ROOT)/libs/dnn/airunner/core/$(ODIR)/airunner.a                                \
    $(SDK_ROOT)/libs/dnn/airunner/importer/$(ODIR)/airunner_importer.a                   \
    $(SDK_ROOT)/libs/dnn/airunner/nodes/apu2/$(ODIR)/airunner_apu2_nodes.a               \
    $(SDK_ROOT)/libs/dnn/airunner/preprocessing/$(ODIR)/airunner_preprocessing.a         \
    $(SDK_ROOT)/libs/dnn/airunner/postprocessing/$(ODIR)/airunner_postprocessing.a       \
      $(SDK_ROOT)/libs/io/frame_io/$(ODIR)/libframe_io.a                          \
    $(SDK_ROOT)/libs/io/sdi/$(ODIR)/libsdi.a                                    \
    $(SDK_ROOT)/libs/isp/cam_generic/user/$(ODIR)/libcamdrv.a                   \
    $(SDK_ROOT)/libs/isp/csi/user/$(ODIR)/libcsidrv.a                           \
    $(SDK_ROOT)/libs/utils/log/$(ODIR)/liblog.a                                 \
    $(SDK_ROOT)/libs/isp/sequencer/user/$(ODIR)/libseqdrv.a                     \
    $(SDK_ROOT)/libs/isp/fdma/user/$(ODIR)/libfdmadrv.a                         \
    $(SDK_ROOT)/isp/graphs/$(ISP_GRAPH)/$(SEQ_FW)/$(ODIR)/lib$(SEQ_FW).a        \
    $(SDK_ROOT)/libs/apex/acf/$(ODIR)/libacf.a                                           \
    $(SDK_ROOT)/libs/apex/icp/$(ODIR)/libicp.a                                           \
    $(SDK_ROOT)/libs/io/frame_io/$(ODIR)/libframe_io.a                                   \
    $(SDK_ROOT)/libs/utils/communications/$(ODIR)/lib_communications.a                   \
    $(SDK_ROOT)/libs/apex/drivers/user/$(ODIR)/libapexdrv.a                              \
    $(SDK_ROOT)/libs/utils/log/$(ODIR)/liblog.a					\
  
ARM_APP_LIBS +=                                                                          \
    $(SDK_ROOT)/libs/utils/oal/user/$(ODIR)/liboal.a                                     \
    $(SDK_ROOT)/libs/utils/common/$(ODIR)/libcommon.a                                    \
    $(SDK_ROOT)/libs/utils/umat/$(ODIR)/libumat.a                                        \
    $(SDK_ROOT)/libs/io/semihost/$(ODIR)/libSemihost.a				\
    $(SDK_ROOT)/libs/io/sdi/$(ODIR)/libsdi.a                                    \
    $(SDK_ROOT)/libs/io/gdi/$(ODIR)/libgdi.a                                    \
    $(SDK_ROOT)/libs/isp/sequencer/user/$(ODIR)/libseqdrv.a                     \
    $(SDK_ROOT)/libs/isp/fdma/user/$(ODIR)/libfdmadrv.a                         \
    $(SDK_ROOT)/libs/utils/oal/user/$(ODIR)/liboal.a                            \
    $(SDK_ROOT)/libs/isp/csi/user/$(ODIR)/libcsidrv.a                           \
    			\
 
ARM_LDOPTS +=                                                                            \
    -lopencv_core                                                                        \
    -lopencv_imgcodecs                                                       \
    -lopencv_imgproc                                                                     \
   -lopencv_videoio                                                         \
    -lavcodec                                                                \
    -lavformat                                                               \
    -lavutil                                                                 \
   -lswresample                                                             \
    -lswscale                                                                \

ARM_APP_LIBS +=                                                              \
    $(SDK_ROOT)/libs/isp/jpegdec/user/$(ODIR)/libjpegdecdrv.a                \
    $(SDK_ROOT)/libs/isp/h264enc/user/$(ODIR)/libh264encdrv.a                \
    $(SDK_ROOT)/libs/isp/h264dec/user/$(ODIR)/libh264decdrv.a                \
    $(SDK_ROOT)/libs/isp/viu/user/$(ODIR)/libviudrv.a                        \
    

##############################################################################
# STANDALONE SPECIFIC INCLUDES
##############################################################################
ifneq (,$(findstring -sa,$(ODIR)))

ARM_APP_LIBS +=                                                                          \
    $(SDK_ROOT)/libs/startup/v234ce_standalone/$(ODIR)/libv234ce.a                       \
    $(SDK_ROOT)/libs/io/i2c/$(ODIR)/libi2c.a                                             \
    $(SDK_ROOT)/libs/io/semihost/$(ODIR)/libSemihost.a                                   \
    $(SDK_ROOT)/libs/io/uartlinflex_io/$(ODIR)/liblinflex.a        

ARM_LDOPTS += -lzlib

endif

##############################################################################
# INTEGRITY SPECIFIC INCLUDES
##############################################################################
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
