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

##############################################################################
# ARM_APP
##############################################################################

ARM_APP = apexdnn_fcw_v3.0

ARM_APP_SRCS =                                                                           \
    main.cpp                                                                             \
    read_image.cpp                                                                       \
    display_image.cpp                                                                    \
    case_mobilenet.cpp                                                                   \
    mobilenet_loop.cpp                                                                   \
	mssd_loop.cpp                                                                        \
    case_mssd.cpp

ARM_INCS +=                                                                              \
    -I../include                                                                         \
    -I$(OPENCV_ROOT)/include                                                             \
    -I$(SDK_ROOT)/demos/airunner/utils/include                                           \
    -I$(SDK_ROOT)/3rdparty/protobuf/include                                              \
    -I$(SDK_ROOT)/include                                                                \
    -I$(SDK_ROOT)/libs/apex/icp/include                                                  \
    -I$(SDK_ROOT)/libs/apex/acf/include                                                  \
    -I$(SDK_ROOT)/libs/io/frame_io/include                                               \
    -I$(SDK_ROOT)/libs/io/dcu/include                                                    \
    -I$(SDK_ROOT)/libs/apex/drivers/user/include                                         \
    -I$(SDK_ROOT)/libs/utils/oal/user/include                                            \
    -I$(SDK_ROOT)/libs/utils/common/include                                              \
    -I$(SDK_ROOT)/libs/dnn/airunner/preprocessing/include                                \

ARM_APP_LIBS =                                                                           \
    $(SDK_ROOT)/libs/apexcv_base/apexcv_core/$(ODIR)/apexcv_core.a                       \
    $(SDK_ROOT)/3rdparty/protobuf/arm64/lib/libprotobuf-lite.a                           \
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
    $(SDK_ROOT)/libs/utils/log/$(ODIR)/liblog.a

ARM_APP_LIBS +=                                                                          \
    $(SDK_ROOT)/libs/utils/oal/user/$(ODIR)/liboal.a                                     \
    $(SDK_ROOT)/libs/utils/common/$(ODIR)/libcommon.a                                    \
    $(SDK_ROOT)/libs/utils/umat/$(ODIR)/libumat.a                                        \
    $(SDK_ROOT)/libs/io/semihost/$(ODIR)/libSemihost.a

ARM_LDOPTS +=                                                                \
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
    -lopencv_objdetect                                                          \

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
