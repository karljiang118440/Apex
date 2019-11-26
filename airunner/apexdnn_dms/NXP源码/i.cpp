

// 从 isp_sonyimx224_csi_dcu_FCW 工程中列出相关显示部分代码

    #ifdef __STANDALONE__
      io::FrameOutputDCU output(1280, 720,  io::IO_DATA_DEPTH_08, CHNL_CNT);
    #else   
      io::FrameOutputV234Fb output(1280, 720, io::IO_DATA_DEPTH_08, CHNL_CNT);
    #endif
     
    vsdk::UMat output_umat = vsdk::UMat(720, 1280, CV_8UC3);
    {
      cv::Mat    output_mat = output_umat.getMat(ACCESS_WRITE | OAL_USAGE_CACHED);
      memset(output_mat.data, 0, 1280*720*3);
  
      outputAPEX.copyTo(output_mat(cv::Rect(0, (720 - outputAPEX.rows)/2, outputAPEX.cols, outputAPEX.rows)));
    }
    output.PutFrame(output_umat);















// Apex2_roi 中 main.cpp 代码

#ifdef __STANDALONE__
#include "../../../data/common/headers/in_grey_256x256.h"
#endif

#if !defined(APEX2_EMULATE) && !defined(__INTEGRITY__)
#ifdef __STANDALONE__
#include "frame_output_dcu.h"
#else // #ifdef __STANDALONE__
#include "frame_output_v234fb.h"
#endif // else from #ifdef __STANDALONE__

#define CHNL_CNT io::IO_DATA_CH3
#endif




#ifdef __STANDALONE__
    io::FrameOutputDCU output(1280, 720, io::IO_DATA_DEPTH_08, CHNL_CNT);
#else
    io::FrameOutputV234Fb output(1280, 720, io::IO_DATA_DEPTH_08, CHNL_CNT);
#endif

    // Output buffer (screen size) and it's mapped version (using cv mat in order to have copyTo functions)
    vsdk::UMat output_umat = vsdk::UMat(720, 1280, VSDK_CV_8UC3);
    {
      cv::Mat output_mat = output_umat.getMat(ACCESS_WRITE | OAL_USAGE_CACHED);
      memset(output_mat.data, 0, 720 * 1280 * 3);

      cv::UMat inRGB(in.rows, in.cols, CV_8UC3);
      cv::UMat out0RGB(in.rows, in.cols, CV_8UC3);
      cv::UMat out1RGB(in.rows, in.cols, CV_8UC3);

      cvtColor((cv::UMat)out0, out0RGB, CV_GRAY2RGB);
      cvtColor((cv::UMat)out1, out1RGB, CV_GRAY2RGB);
      cvtColor((cv::UMat)in, inRGB, CV_GRAY2RGB);

      cv::rectangle(out0RGB, cv::Point(0, 0), cv::Point(out0.cols - 1, out0.rows - 1), cv::Scalar(255, 255, 255));
      cv::rectangle(out1RGB, cv::Point(0, 0), cv::Point(out1.cols - 1, out1.rows - 1), cv::Scalar(255, 255, 255));
      inRGB.copyTo(output_mat(cv::Rect(0, 232, 256, 256)));
      out0RGB.copyTo(output_mat(cv::Rect(300, 232, 256, 256)));
      out1RGB.copyTo(output_mat(cv::Rect(600, 232, 256, 256)));
    }

    output.PutFrame(output_umat);










// 从 isp_sonyimx224_csi_dcu_FCW 工程中列出相关显示部分代码

	//*** Init DCU Output ***
#ifdef __STANDALONE__
  io::FrameOutputDCU lDcuOutput(WIDTH, HEIGHT, io::IO_DATA_DEPTH_08, CHNL_CNT);
#else  // #ifdef __STANDALONE__
  // setup Ctrl+C handler
  if(SigintSetup() != SEQ_LIB_SUCCESS)
  {
    VDB_LOG_ERROR("Failed to register Ctrl+C signal handler.");
    return -1;
  }

  printf("Press Ctrl+C to terminate the demo.\n");

  io::FrameOutputV234Fb lDcuOutput(WIDTH, HEIGHT, io::IO_DATA_DEPTH_08, CHNL_CNT);
#endif // else from #ifdef __STANDALONE__


//=============================


lDcuOutput.PutFrame(lFrame.mUMat);

