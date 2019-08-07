
//apexcv_pro_hog.h

class Hog:public ApexcvHostBaseBaseClass





//HOGSVMDetector_Types.hpp
struct Fd_HogData:public MessageBase
{
  std::vector<ROI> mFaceRois;
  uint64_t         mHogTicks[2];
  uint64_t         

};
















// HoGSVMDetector.hpp


/***************HOG****************/
uint32_t mNumberofSceneResizes_HOG;
Fd_FrameSize mSceneResizes_HOG[gcNumberOfSceneResizes];

vsdk::Umat mHogScores_umat;
uint32_t MSceneResizesHogCounters[gcNumberOfSceneResizes];

apexcv::Hog mHog;
ref::Hog mHogRef;
std::vector<ROI>mFaceRois;
bool mWeUseApexHog;
bool mIsApexInitialized;
bool mIsConfigured_HOG;

private:
  LIB_RESULT HogInit(const uint32_t cNumberOfSceneResizes,
                     const Fd_FrameSize* const cpcSceneResizes);

public:
  LIB_RESULT FaceDetectorInit(char* svmFilePath);
  int FaceDetectorProcess(vsdk::UMat&grayimage_umat,
                          cv::Rect& objectBox);




















  //hog_config.hpp

  #ifndef HOGCONFIG_H
  #define HOGCONFIG_H

  #define N_BINS 8




  //HoGSVMDetector_Configuration.hpp


/***************************************************************************/
// HogTask
/***************************************************************************/
//const char gcFilenameSvm[] = "svm_function_ocv.bin";

//该部分使用的是 bin 文件？使用opencv 的函数得到的结果？ 

const ACF_APU_CFG gcApexCvHogAcfApuCfg = ACF_APU_CFG__DEFAULT;
const int32_t gcApexCvHogApexId = 1;
















