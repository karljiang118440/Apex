/*****************************************************************************
*
* Freescale Confidential Proprietary
*
* Copyright (c) 2016 Freescale Semiconductor;
* All Rights Reserved
*
*****************************************************************************
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
****************************************************************************/

#ifndef __STANDALONE__
  #include <signal.h>
#endif // #ifdef __STANDALONE__
#include <string.h>

#ifdef __STANDALONE__
  #include "frame_output_dcu.h"
  #define CHNL_CNT io::IO_DATA_CH3
#else // #ifdef __STANDALONE__
  #include "frame_output_v234fb.h"
  #define CHNL_CNT io::IO_DATA_CH3
#endif // else from #ifdef __STANDALONE__  

#include "sdi.hpp"
#include "mipi_simple_c.h"

#include "vdb_log.h"




// add my code 2018.8.27 -----------------------------------------------------------------

#include<opencv2/opencv.hpp>
#include<dlib/opencv.h>
#include<dlib/image_processing/frontal_face_detector.h>
#include<dlib/image_processing/render_face_detections.h>
#include<dlib/image_processing.h>
#include<iostream>
#include<algorithm>



using namespace dlib;
using namespace std;

//----------------------------------------------------------end--------------------2018.8.28------------



// �ж���ֵ��Ĭ��0.3�����EAR������������Ϊ�۾��������ģ����С����������Ϊ�۾��Ǳպϵ�
#define EYE_AR_THRESH 0.21
// ��EARС����ֵʱ����������֡һ������գ�۶�����ֻ��С����ֵ��֡���������ֵ������Ϊ��ǰ�۾��Ǳպϵģ�������գ�۶�����������Ϊ�������
#define EYE_AR_CONSEC_FRAMES  1
#define PERCLOS_EVALUATE_TIME_S 15
#define PERCLOS_THRESH 0.4  //����0.15�� ��ƣ��
#define BLINK_FREQ_MINUTE 9 //գ��Ƶ��С�� 7��/���� ==> ƣ��

// ��ʾ display
#define SCREEN_LEFT_X_AXIS  25  //��Ļ��� x ����
#define SCREEN_LEFT_Y_AXIS_FACE_DETECTION_TIME  430  //��Ļ����������ʱ�� y ����
#define SCREEN_LEFT_Y_AXIS_BLINK_COUNT 400  //��Ļ���գ�۴��� y ����
#define SCREEN_LEFT_Y_AXIS_FACE_NO  180  //û�м�⵽������ʾ
#define SCREEN_LEFT_Y_AXIS_LONG_DIS  150  //��ʾ state

#define SCREEN_LEFT_Y_AXIS_FATIGUE_DIS0  220  
#define SCREEN_LEFT_Y_AXIS_FATIGUE_DIS1  250  
#define SCREEN_LEFT_Y_AXIS_FATIGUE_DIS2  280  
#define SCREEN_LEFT_Y_AXIS_FATIGUE_DIS3  310  
#define SCREEN_LINE_SEG 30

//putText para set
#define FONT_SIZE_DIS 0.5
//----------------------------------------------


#define scaleTony 4

string iniState = "STATE: ";
string iniDriverState = "No Face Detected !";
string deteDriverState = "Face Detected !";

cv::Point point_arr[4] = { cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_FATIGUE_DIS0), cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_FATIGUE_DIS1), cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_FATIGUE_DIS2), cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_FATIGUE_DIS3) };



bool flag_one = true;
bool flag_two = true;
bool flag_three = true;
bool flag_four = true;
bool flag_big = true;
int tony_flag;
std::vector<string>fa;
string tmp_display_fatigue;

int test_e5 = 0;

char fataguePara[4][200] = { { "%s ; You have fatigued !" },
{ "%s ; You have fatigued !" },
{ "%s ; You have fatigued !" },
{ "%s ; You have fatigued !" } };

int array2[4] = { 0, 1, 2, 3 };



string doubleTwoRound(double dVal);
double calcTwoNormIsEuclid(cv::Point p1, cv::Point p2);
double eyeAspectRatio(std::vector<cv::Point>eye);
int round_double(double number);
string getCurrentTime();
cv::Mat scrollScreen(int fatigue, cv::Mat& temp);
std::vector<string> characterStingSplit(string pend_string);


//-----------------------------------------------------------------------2018.9.4----------------------------------------





//***************************************************************************
// constants
//***************************************************************************

// Possible to set input resolution (must be supported by the DCU)
#define WIDTH           1280 ///< width of DDR buffer in pixels
#define HEIGHT          720 ///< height of DDR buffer in pixels
#define DDR_BUFFER_CNT  3    ///< number of DDR buffers per ISP stream

//***************************************************************************

#define FRM_TIME_MSR 300 ///< number of frames to measure the time and fps

//***************************************************************************
// macros
//***************************************************************************

#ifdef __STANDALONE__
//extern SEQ_Buf_t  producerless_buffer_1;
extern "C" {
  unsigned long get_uptime_microS(void);
}

#define GETTIME(time)   (*(time)=get_uptime_microS())
#else // ifdef __STANDALONE__
#define GETTIME(time) \
  { \
  struct timeval lTime; gettimeofday(&lTime,0); \
  *time=(lTime.tv_sec*1000000+lTime.tv_usec);   \
  }
#endif // else from #ifdef __STANDALONE__

//***************************************************************************
// types
//***************************************************************************
struct AppContext
{
  sdi_grabber *mpGrabber;      ///< pointer to grabber instance
  sdi_FdmaIO  *mpFdma;         ///< pointer to fdma object
  
  // ** event counters and flags **
  bool     mError;            ///< to signal ISP problems
  uint32_t mFrmDoneCnt;       ///< number of frames done events
  uint32_t mFrmCnt;           ///< number of frames grabbed by the app  
}; // struct AppContext

/************************************************************************/
/** User defined call-back function for Sequencer event handling.
  * 
  * \param  aEventType defines Sequencer event type
  * \param  apUserVal  pointer to any user defined object 
  ************************************************************************/
static void SeqEventCallBack(uint32_t aEventType, void* apUserVal);

/************************************************************************/
/** Prepare everything before executing the main functionality .
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, <0 otherwise 
  ************************************************************************/
static int32_t Prepare(AppContext &arContext);

/************************************************************************/
/** Initial setup of application context.
  * 
  * \param arContext structure capturing the context of the application
  ************************************************************************/
static void ContextInit(AppContext &arContext);

/************************************************************************/
/** Prepares required libraries.
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, != 0 otherwise 
  ************************************************************************/
static int32_t LibsPrepare(AppContext &arContext);

/************************************************************************/
/** Prepares DDR buffers.
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, != 0 otherwise 
  ************************************************************************/
static int32_t DdrBuffersPrepare(AppContext &arContext);

/************************************************************************/
/** Execute main functionality of the application.
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, <0 otherwise 
  ************************************************************************/
static int32_t Run(AppContext &arContext);

/************************************************************************/
/** Cleanup all resources before application end.
  * 
  * \param arContext structure capturing the context of the application
  * 
  * \return 0 if all ok, <0 otherwise 
  ************************************************************************/
static int32_t Cleanup(AppContext &arContext);


#ifndef __STANDALONE__
/************************************************************************/
/** SIGINT handler.
  * 
  * \param  aSigNo 
  ************************************************************************/
static void SigintHandler(int);

/************************************************************************/
/** SIGINT handler.
  * 
  * \param  aSigNo 
  * 
  * \return SEQ_LIB_SUCCESS if all ok
  *         SEQ_LIB_FAILURE if failed
  ************************************************************************/
static int32_t SigintSetup(void);

//*****************************************************************************
// static variables
//*****************************************************************************

static bool sStop = false; ///< to signal Ctrl+c from command line

#endif // #ifndef __STANDALONE__

int main(int, char **)
{
  int lRet = 0;

  AppContext lContext;
     
  //*** process command line parameters ***
  printf("\n**************************************************************\n"
         "** Sony imx224 csi -> dcu demo\n"
         "** Description:\n"
         "**  o Sony imx224 camera (on MipiCsi_0) expected as image input.\n"
         "**  o ISP does simple debayering.\n"
         "**  o Resulting RBG 1280x720 image is displayed live using DCU.\n"
         "**\n"
         "** Usage:\n"
         "**  o no cmd line parameters available.\n"
         "**\n"
         "**************************************************************\n\n");
#ifndef __STANDALONE__  
  fflush(stdout);  
  msleep(1000);
#endif // ifndef __STANDALONE__
  
  if(Prepare(lContext) == 0)
  {
    if(Run(lContext) != 0)
    {
      printf("Demo execution failed.\n");
      lRet = -1;
    } // if Run() failed
  } // if Prepare() ok
  else
  {
    printf("Demo failed in preparation phase.\n");
    lRet = -1;
  }// else from if Prepare() ok
  
  if(Cleanup(lContext) != 0)
  {
    printf("Demo failed in cleanup phase.\n");
    lRet = -1;
  } // if cleanup failed

  return lRet;
} // main()

//***************************************************************************

static int32_t Prepare(AppContext &arContext)
{  
  // init app context
  ContextInit(arContext);
  
  // enable LIBS
  if(LibsPrepare(arContext) != 0)
  {
    printf("Failed to prepare libraries.\n");
    return -1;
  } // if failed to configure decoder
  // enable OAL
  
#ifndef __STANDALONE__  
  
#endif // #ifndef __STANDALONE__ 
  
  if(DdrBuffersPrepare(arContext) != 0)
  {
    printf("Failed to prepare DDR buffers.\n");
    return -1;
  } // if fialed to prepare DDR buffers  


  // *** prestart grabber ***
  if(arContext.mpGrabber->PreStart() != LIB_SUCCESS)
  {
    printf("Failed to prestart the grabber.\n");
    return -1;
  } // if PreStart() failed
  
  if(arContext.mpGrabber->SeqEventCallBackInstall
  (
    &SeqEventCallBack, 
    &arContext
  ) != LIB_SUCCESS)
  {
    printf("Failed to install Sequencer event callback.\n");
    return -1;
  } // if callback setup failed
   
  return 0;
} // Prepare()

//***************************************************************************

static void ContextInit(AppContext &arContext)
{
  arContext.mpGrabber   = NULL;      
  arContext.mpFdma      = NULL; 
  arContext.mError      = false;
  arContext.mFrmCnt     = 0;
  arContext.mFrmDoneCnt = 0;
} // ContextInit()

//***************************************************************************

static int32_t LibsPrepare(AppContext &arContext)
{
  // *** Initialize SDI ***
  if(sdi::Initialize(0) != LIB_SUCCESS)
  {
    printf("Failed to initialzie SDI.\n");
    return -1;
  } // if failed to initialize SDI
  
  // create grabber
  arContext.mpGrabber = new(sdi_grabber);  
  if(arContext.mpGrabber == NULL)
  {    
    printf("Failed to create sdi grabber.\n");
    return -1;
  } // if failed to create grabber
  
  if(arContext.mpGrabber->ProcessSet(gpGraph, &gGraphMetadata) != LIB_SUCCESS)
  {
    printf("Failed to set ISP graph to grabber.\n");
    return -1;
  } // if ISP graph not set
  
  // get IOs
  arContext.mpFdma = (sdi_FdmaIO*)arContext.mpGrabber->IoGet(SEQ_OTHRIX_FDMA);    
  if(arContext.mpFdma == NULL)
  {
    printf("Failed to get FDMA object.\n");
    return -1;
  } // if no FDMA object
  
  return 0;
} // LibsPrepare(AppContext &arContext)

//***************************************************************************

static int32_t DdrBuffersPrepare(AppContext &arContext)
{
  // *** RGB full buffer array ***
  // modify DDR frame geometry to fit display output
  SDI_ImageDescriptor lFrmDesc;
  lFrmDesc = SDI_ImageDescriptor(WIDTH, HEIGHT, RGB888);

  if(arContext.mpFdma->DdrBufferDescSet(FDMA_IX_FastDMA_Out_MIPI_SIMPLE, 
                                        lFrmDesc) != LIB_SUCCESS)

/*										
  if(arContext.mpFdma->DdrBufferDescSet(FDMA_IX_FastDMA_Out, 
                                        lFrmDesc) != LIB_SUCCESS)										
*/										
  {
    printf("Failed to set image descriptor.\n");
    return -1;
  } // if frame descriptor setup failed
  
  // allocate DDR buffers
  if(arContext.mpFdma->DdrBuffersAlloc(DDR_BUFFER_CNT) != LIB_SUCCESS)
  {
    printf("Failed to allocate DDR buffers.\n");
    return -1;
  } // if ddr buffers not allocated
  
  return 0;
} // DdrBuffersPrepare(AppContext &arContext)

//***************************************************************************

static int32_t Run(AppContext &arContext)
{  
  //*** Init DCU Output ***
#ifdef __STANDALONE__
  io::FrameOutputDCU lDcuOutput(
                      WIDTH, 
                      HEIGHT, 
                      io::IO_DATA_DEPTH_08, 
                      CHNL_CNT);
#else // #ifdef __STANDALONE__
  // setup Ctrl+C handler
  if(SigintSetup() != SEQ_LIB_SUCCESS) 
  {
    VDB_LOG_ERROR("Failed to register Ctrl+C signal handler.");
    return -1;
  }
  
  printf("Press Ctrl+C to terminate the demo.\n");
  
  io::FrameOutputV234Fb lDcuOutput(
                          WIDTH, 
                          HEIGHT, 
                          io::IO_DATA_DEPTH_08, 
                          CHNL_CNT);
#endif // else from #ifdef __STANDALONE__
  
  unsigned long lTimeStart = 0, lTimeEnd = 0, lTimeDiff = 0;
  
  // *** start grabbing ***
  GETTIME(&lTimeStart);
  if(arContext.mpGrabber->Start() != LIB_SUCCESS)
  {
    printf("Failed to start the grabber.\n");
    return -1;
  } // if Start() failed
  
  SDI_Frame lFrame;
  // *** grabbing/processing loop ***  

    //----------------------------------2018.8.28
/*
       double calc_time;
       calc_time = (double)cv::getTickCount();
  
       frontal_face_detector detector = get_frontal_face_detector();
       shape_predictor pose_model;
       deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;

       calc_time = (double)cv::getTickCount() - calc_time;
       printf("load face detection and pose estimation models:%g ms\n", calc_time * 1000 / cv::getTickFrequency());
*/
       //--------------------2018.8.28---end---------------------



/*
  double calc_time;
  calc_time = (double)cv::getTickCount();

  cv::CascadeClassifier faceCascade;
  faceCascade.load("haarcascade_frontalface_alt.xml");
  dlib::shape_predictor pose_model;
  deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;

  calc_time = (double)cv::getTickCount() - calc_time;
  printf("load pose estimation models : %g ms\n", calc_time * 1000 / cv::getTickFrequency());
*/
  //------------------------------------2018.9.4----------------------------------------------



  // read wpi_logo
  cv::Mat logo = cv::imread("logo_wpi.jpg");
  if (!logo.data)
  {
	  std::cout << "��ȡԭʼlogoͼʧ�ܣ�" << endl;
	  return -1;
  }

  //cv::Mat temp = cv::imread("C:/Users/Administrator/Desktop/face_two.jpg");
  double calc_time;
  calc_time = (double)cv::getTickCount();

  cv::CascadeClassifier faceCascade;
  faceCascade.load("haarcascade_frontalface_alt.xml");
  dlib::shape_predictor pose_model;
  deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;

  calc_time = (double)cv::getTickCount() - calc_time;
  printf("load pose estimation models : %g ms\n", calc_time * 1000 / cv::getTickFrequency());


  int ear_count = 0;
  int total_eye_blink = 0;
  int count_frame = 0;  // ����֡��ȷ����30s ��֡�� = 30 * 30 �������� 1s = 30fps ��
  double perclos; //p80
  double blinkFreq; //գ��Ƶ��
  double clac_sum_time = 0.0; //������ʱ���Ƿ�ﵽ 30s
  int t30s = 0;
  int unitTimeEyeCloseFrame_n = 0;
  int fatigue = 0; //ƣ�ͼ���
  int ear_close_state = 0;
  int gCount=0;
  int criticalValue = 0;

  //-----------------------------------------------2018.9.4---------------------end---------------------

  for(;;)
  {
	  lFrame = arContext.mpGrabber->FramePop();
	  if(lFrame.mUMat.empty())
	  {
		  printf("Failed to grab image number %u\n", arContext.mFrmCnt);
		  arContext.mError = true;
		  break;
	  } // if pop failed
	  #if 0
	  if(gCount!=2)
	  {
	     gCount++;
	     if(arContext.mpGrabber->FramePush(lFrame) != LIB_SUCCESS)
	       {
	       printf("Failed to push image number %u\n", arContext.mFrmCnt);
		 arContext.mError = true;
		   break;
	       } // if push failed     
	     continue;
	  }
	  else
	  {
	    gCount=0;
	  }
	#endif


	  //try
	  //{

	  vsdk::UMat output_umat = vsdk::UMat(HEIGHT, WIDTH, CV_8UC3);

	  cv::Mat temp = lFrame.mUMat.getMat(vsdk::ACCESS_RW | OAL_USAGE_CACHED);

	  //--------------------------------------------------2018.9.4-----------------------------------------------

	  double cam_fps;
	  double calc_30s;
	  cam_fps = (double)cv::getTickCount();
	  calc_30s = (double)cv::getTickCount();
	  std::vector<cv::Point>leftEye, rightEye;	
	  // Grab a frame  
	  cv::Mat face_one_gray;

                       cv::flip(temp, temp, 0);
	  cv_image<bgr_pixel> cimg(temp);



                       cv::Mat  face_one_scale;


                       
                      // cv::pyrDown(temp, face_one_scale);
		      // cv::pyrDown(face_one_scale, face_one_scale);







	  std::vector<cv::Rect>face_rect;
	  std::vector<dlib::full_object_detection> shapes;

	  string titleDms = "DMS On NXP S32V234";
	  cv::putText(temp, titleDms, cv::Point(temp.cols / 2 - titleDms.size() / 2 * 20, 50), CV_FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(0, 0, 255), 2);
	  cv::putText(temp, iniState, cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_LONG_DIS), CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 0, 255), 2);

	  //------add wpi logo------
	  cv::Mat imageROI = temp(cv::Rect(temp.cols - logo.cols, temp.rows - logo.rows, logo.cols, logo.rows)); //��ԭͼ�пٳ���������Rect��һ��������ʾ�������ϽǶ�������꣬���ڶ�λ��������������ʾ���еĿ��͸�

	  cv::addWeighted(imageROI, 0.2, logo, 0.8, 0.0, imageROI);//dst = src1[I]*alpha+ src2[I]*beta + gamma����һ���ĸ��������Ǹ���Ȩ�أ���5���������ǹ�ʽ�е�ƫִ����gamma��
	  //-----end add wpi logo------

	 // cv::cvtColor(face_one_scale, face_one_scale, CV_BGR2GRAY);
	 // cv::equalizeHist(face_one_scale, face_one_scale);

	  cv::cvtColor(temp, face_one_gray, CV_BGR2GRAY);
	  cv::equalizeHist(face_one_gray, face_one_gray);

                       cv::pyrDown(face_one_gray, face_one_scale);
		       cv::pyrDown(face_one_scale, face_one_scale);


	  //faceCascade.detectMultiScale(face_one_gray, face_rect, 1.1, 3, 0, cv::Size(30, 30));

	  double face_detect_calc_time;
	  face_detect_calc_time = (double)cv::getTickCount();

	  faceCascade.detectMultiScale(face_one_scale, face_rect);

	  face_detect_calc_time = (double)cv::getTickCount() - face_detect_calc_time;
	  double face_detect_time = face_detect_calc_time * 1000 / cv::getTickFrequency();
//	  printf("face_detect: %.2f ms\n", face_detect_time);
	  string ds = "detect rate(ms): " + doubleTwoRound(face_detect_time);

	  // �����ⲻ������������ʾʱ�䣻�����⵽��������ʾʱ��
	  // if no detected face, dont't display time; 
	  if (face_rect.size() != 0)
	  {
		  cv::putText(temp, ds, cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_FACE_DETECTION_TIME), CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 0, 255), 2);
	  
            
		  cv::putText(temp, deteDriverState, cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_FACE_NO), CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 0, 255), 2);

	  }



	  std::vector<unsigned int>areaRect;

	  for (unsigned int i = 0; i < face_rect.size(); i++)
	  {
		 // cv::rectangle(temp, cv::Point(face_rect[i].x*scaleTony, face_rect[i].y*scaleTony), cv::Point(face_rect[i].x*scaleTony + face_rect[i].width*scaleTony, face_rect[i].y*scaleTony + face_rect[i].height*scaleTony), cv::Scalar(0, 255, 0), 2);
		  dlib::rectangle r((long)face_rect[i].x*scaleTony, (long)face_rect[i].y*scaleTony, (long)(face_rect[i].x*scaleTony + face_rect[i].width*scaleTony), (long)(face_rect[i].y*scaleTony + face_rect[i].height*scaleTony));

		  // Find the pose of each face.
		  shapes.push_back(pose_model(cimg, r));

		 unsigned int rectArea = face_rect[i].width * face_rect[i].height;

		  areaRect.push_back(rectArea);

	 
	 }


	  //-----------------add =------2018.9.5-------------------


	  //add �����ж�
	  std::vector<unsigned int>::iterator biggest = std::max_element(std::begin(areaRect), std::end(areaRect));
	  for (unsigned int i = 0; i < face_rect.size(); i++)
	  {

		 unsigned int max_index = std::distance(std::begin(areaRect), biggest);
		  if (i == max_index)
		  {
			
			
		  cv::rectangle(temp, cv::Point(face_rect[max_index].x*scaleTony, face_rect[max_index].y*scaleTony), cv::Point(face_rect[max_index].x*scaleTony + face_rect[max_index].width*scaleTony, face_rect[max_index].y*scaleTony + face_rect[max_index].height*scaleTony), cv::Scalar(0, 255, 0), 2);
		//	cv::rectangle(temp, cv::Point(face_rect[max_index].x, face_rect[max_index].y), cv::Point(face_rect[max_index].x + face_rect[max_index].width, face_rect[max_index].y + face_rect[max_index].height), cv::Scalar(0, 255, 0), 2);
		  }
		  else
		  {
			
			
		  cv::rectangle(temp, cv::Point(face_rect[i].x*scaleTony, face_rect[i].y*scaleTony), cv::Point(face_rect[i].x*scaleTony + face_rect[i].width*scaleTony, face_rect[i].y*scaleTony + face_rect[i].height*scaleTony), cv::Scalar(125, 125, 125), 2);
		//	cv::rectangle(temp, cv::Point(face_rect[i].x, face_rect[i].y), cv::Point(face_rect[i].x + face_rect[i].width, face_rect[i].y + face_rect[i].height), cv::Scalar(125, 125, 125), 2);
		  }
	  }




//--------------------2018.9.5------------------end----------------------






	  if (!shapes.empty())
	  {
		  for (unsigned int j = 0; j < shapes.size(); j++)
		  {
			  //cv::putText(temp, deteDriverState, cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_FACE_NO), CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 0, 255), 2);
                          if(j == std::distance(std::begin(areaRect), biggest))
			  {
			  for (unsigned int i = 0; i < 68; i++)
			  {
				  //cv::putText(temp, to_string(i), cv::Point(shapes[j].part(i).x(), shapes[j].part(i).y()), CV_FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 0, 0));

				  //extract left eye and right eye
				  if (i >= 36 && i <= 41)
				  {
					  leftEye.push_back(cv::Point(shapes[j].part(i).x(), shapes[j].part(i).y()));
					  circle(temp, cv::Point(shapes[j].part(i).x(), shapes[j].part(i).y()), 2, cv::Scalar(255, 0, 0), -1);

				  }
				  if (i >= 42 && i <= 47)
				  {

					  circle(temp, cv::Point(shapes[j].part(i).x(), shapes[j].part(i).y()), 2, cv::Scalar(255, 0, 0), -1);
					  rightEye.push_back(cv::Point(shapes[j].part(i).x(), shapes[j].part(i).y()));
				  }
				 // circle(temp, cv::Point(shapes[j].part(i).x(), shapes[j].part(i).y()), 3, cv::Scalar(0, 0, 255), -1);

			  }
			  // calc eye aspect ratio
			  double leftEyeEar = eyeAspectRatio(leftEye);
			  double rightEyeEar = eyeAspectRatio(rightEye);

			  // average the eye aspect ratio together for both eyes
			  double calcLeftRightEyeEar = (leftEyeEar + rightEyeEar) / 2.0;

			 FILE *file_in = fopen("leftEyeEar.txt", "a");
			  fprintf(file_in, "leftEyeEar: %.2f, rightEyeEar: %.2f, ratio: %.2f\n", leftEyeEar, rightEyeEar, calcLeftRightEyeEar);
			  fclose(file_in);
                         
			  if (calcLeftRightEyeEar < EYE_AR_THRESH) //С�� 0.3 ��Ϊһ��գ��
			  {
				  ear_count += 1;
				  printf("close = %d \n", ear_count);
				  ear_close_state += 1;  //���� perclos ��
			  }
			  else
			  {
				  if (ear_count >= EYE_AR_CONSEC_FRAMES)
					  total_eye_blink += 1;
				  ear_count = 0;
				  printf("dtected = %d \n", total_eye_blink);

			  }
			  cv::putText(temp, "Blinks: " + to_string(total_eye_blink), cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_BLINK_COUNT), CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 0, 255), 2);
			  cv::putText(temp, "EAR: " + to_string(calcLeftRightEyeEar), cv::Point(25, 460), CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 0, 255), 2);
			  }
		  }
		  //	cv::putText(temp, "Currently in a state of fatigue : ", cv::Point(25, 50), CV_FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
		  cam_fps = (double)cv::getTickCount() - cam_fps;
		//  printf("one frame time : %g \n", (cam_fps * 1000 / cv::getTickFrequency()));
		 // FILE *file_in_fps = fopen("fps.txt", "a");
		 // fprintf(file_in_fps, "fps: %.2f, one frame time : %.2f ms\n", 1.0 / (cam_fps / cv::getTickFrequency()), (cam_fps * 1000 / cv::getTickFrequency()));
		 // fclose(file_in_fps);


		  // calc PERCLOS  f = n / N  blinkfreq = n / (t * N)
		  count_frame++;
		  calc_30s = (double)cv::getTickCount() - calc_30s;
		  double calc_one_frame_time_s = calc_30s / cv::getTickFrequency();
		  clac_sum_time += calc_one_frame_time_s;
		 // FILE *file_in_clac_sum_time = fopen("clac_sum_time.txt", "a");
		 // fprintf(file_in_clac_sum_time, "clac_sum_time: %.2f s\n", clac_sum_time);
		 // fclose(file_in_clac_sum_time);

		  if (round_double(clac_sum_time) == PERCLOS_EVALUATE_TIME_S)
		  {
			  t30s++;
			  if (t30s == 1)
			  {
				  unitTimeEyeCloseFrame_n = ear_close_state - criticalValue;
			  }
			  else
			  {
				  unitTimeEyeCloseFrame_n = ear_close_state - unitTimeEyeCloseFrame_n;
			  }

			  // ���� 30s գ��10�Σ�һ�� 3 ֡���ܹ�30֡
			  //int eyeCloseFrame_n = unitTimeEyeCloseFrame_n * EYE_AR_CONSEC_FRAMES;
			  int eyeCloseFrame_n = unitTimeEyeCloseFrame_n;
			  //int count_N = PERCLOS_EVALUATE_TIME_S * 30;
			  int count_N = count_frame;
			  perclos = (double)eyeCloseFrame_n / count_N;
			  // blink frequency
			  blinkFreq = total_eye_blink / (calc_one_frame_time_s * count_frame);
			  if (perclos > PERCLOS_THRESH)
			  {
				  //cv::putText(temp, "Perclos: " + doubleTwoRound(perclos) + "; Currently in a state of fatigue !", cv::Point(25, 33), CV_FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
				  fatigue++;
				  //cv::putText(temp, to_string(fatigue), cv::Point(25, 450), CV_FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);

			  }

			  else
			  {
				  if (blinkFreq < (BLINK_FREQ_MINUTE / 60))
				  {
					  //cv::putText(temp, "BlinkFreq: " + doubleTwoRound(blinkFreq) + "; Currently in a state of fatigue !", cv::Point(25, 41), CV_FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
					  fatigue++;
					  //cv::putText(temp, to_string(fatigue), cv::Point(25, 450), CV_FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
				  }
			  }

		//	  FILE *file_i = fopen("perblink.txt", "a");
		//	  fprintf(file_i, "perclose: %.2f , blinkFreq: %.2f, count_frame: %d, how30s: %d, eyeCloseFrame_n: %d, total_eye_blink: %d, fatigue: %d\n", perclos, blinkFreq, count_frame, t30s, eyeCloseFrame_n, total_eye_blink, fatigue);
		//	  fclose(file_i);
			  if (t30s == INT_MAX)
			  {
				  t30s = 0;
			  }
			  if(t30s == 0)
			  {
			      criticalValue = ear_close_state;
			  }
			  count_frame = 0;
			  clac_sum_time = 0.0;
		  }

		  if (fatigue > 0)
		  {
			  cv::Mat scrollImg = scrollScreen(fatigue, temp);
			  scrollImg.copyTo(temp);

		  }
	  }
	  else // ���ǿ�ִ������ ����ִ�д�  ��⵽����ִ������
	  {
		  cv::putText(temp, iniDriverState, cv::Point(SCREEN_LEFT_X_AXIS, SCREEN_LEFT_Y_AXIS_FACE_NO), CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 0, 255), 2);
	  }

	  //	cv::putText(temp, to_string(fatigue), cv::Point(400, 50), CV_FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
	//  imshow("Results_multiview_reinforce", temp);
	  //cv::waitKey(0);
 


//----------------------------2018.9.4-------------------------end------------------------------------------

/*
                       cv::flip(temp, temp, 0);
                       cv::Mat face_one_gray, face_one_scale;
		       cv_image<bgr_pixel> cimg(temp);

		       std::vector<cv::Rect>face_rect;
		       std::vector<dlib::full_object_detection> shapes;

                       
                       cv::pyrDown(temp, face_one_scale);
		       cv::pyrDown(face_one_scale, face_one_scale);
		       cv::cvtColor(face_one_scale, face_one_gray, CV_BGR2GRAY);
		       cv::equalizeHist(face_one_gray, face_one_gray);
		       //faceCascade.detectMultiScale(face_one_gray, face_rect, 1.1, 3, 0, cv::Size(30, 30));

		       double calc_time;
		       calc_time = (double)cv::getTickCount();

		       faceCascade.detectMultiScale(face_one_gray, face_rect);

		       calc_time = (double)cv::getTickCount() - calc_time;
		       printf("detect rate : %g ms\n", calc_time * 1000 / cv::getTickFrequency());

		       //if (face_rect.size() == 0)
		       //{
		       //       continue;
		       //}
		       for (unsigned int i = 0; i < face_rect.size(); i++)
		       {
			       //cv::rectangle(temp, cv::Point(face_rect[i].x, face_rect[i].y), cv::Point(face_rect[i].x + face_rect[i].width, face_rect[i].y + face_rect[i].height), cv::Scalar(0, 0, 255), 2);
			       dlib::rectangle r((long)face_rect[i].x*4, (long)face_rect[i].y*4, (long)(face_rect[i].x*4 + face_rect[i].width*4), (long)(face_rect[i].y*4 + face_rect[i].height*4));

			       // Find the pose of each face.
			       shapes.push_back(pose_model(cimg, r));                                                                                          
		       }   

		       if (!shapes.empty())
		       {
			       for (unsigned int j = 0; j < shapes.size(); j++)
			
				       {
					       for (unsigned int i = 0; i < 68; i++) 
					       {
						       circle(temp, cvPoint(shapes[j].part(i).x(), shapes[j].part(i).y()), 3, cv::Scalar(0, 0, 255), -1);
						       //cv::putText(temp, to_string(i), cv::Point(shapes[j].part(i).x(), shapes[j].part(i).y()), CV_FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 0, 0));
						       //cout << "the " << i << " point��" << cv::Point(shapes[j].part(i).x(), shapes[j].part(i).y()) << endl;
					       }
				       }

			       }  */
 // --------------------------2018.8.27 by Tony ----------------------------------end----------------------------------------------






//------------------------------------------------------------------------------2018.9.4--------------------------endl----------------------

		       memcpy( (char*)output_umat.getMat(vsdk::ACCESS_RW | OAL_USAGE_CACHED).data, (char*)temp.data, temp.rows * temp.cols * temp.channels() );
   // ------------------------------------------------------------------------------------------------end-----------------------
    arContext.mFrmCnt++;
       
    lDcuOutput.PutFrame(output_umat);

    if(arContext.mpGrabber->FramePush(lFrame) != LIB_SUCCESS)
    {
      printf("Failed to push image number %u\n", arContext.mFrmCnt);
      arContext.mError = true;
      break;
    } // if push failed    
    
    if((arContext.mFrmCnt%FRM_TIME_MSR) == 0)
    {
      GETTIME(&lTimeEnd);
      lTimeDiff  = lTimeEnd - lTimeStart;
      lTimeStart = lTimeEnd;
      
      printf("%u frames took %lu usec (%5.2ffps)\n",
             FRM_TIME_MSR,
             lTimeDiff,
            (FRM_TIME_MSR*1000000.0)/((float)lTimeDiff));
    }// if time should be measured
#ifndef __STANDALONE__  
    if(sStop)
    {
      break; // break if Ctrl+C pressed
    } // if Ctrl+C
#endif //#ifndef __STANDALONE__  
  } // for ever
    
  return 0;
} // Run()

//***************************************************************************

static int32_t Cleanup(AppContext &arContext)
{
  int32_t lRet = 0;
  
  if(arContext.mpGrabber != NULL)
  {
    if(arContext.mpGrabber->Stop())
    {
      printf("Failed to stop the grabber.\n");
      lRet = -1;
    } // if grabber stop failed
    
    if(arContext.mpGrabber->Release())
    {
      printf("Failed to release grabber resources.\n");
      lRet = -1;
    } // if grabber resources not released
    
    delete(arContext.mpGrabber);
    arContext.mpGrabber = NULL;
  } // if grabber exists
  
#ifdef __STANDALONE__
  for(;;);  // *** don't return *** 
#endif // #ifdef __STANDALONE__    
  
  if(sdi::Close(0) != LIB_SUCCESS)
  {
    printf("Failed to terminate use of SDI.\n");
    lRet = -1;
  } // if SDI use termination failed
  
  return lRet;
} // Cleanup()

//***************************************************************************

static void SeqEventCallBack(uint32_t aEventType, void* apUserVal)
{
  AppContext *lpAppContext = (AppContext*)apUserVal;
  
  if(lpAppContext)
  {  
    if(aEventType == SEQ_MSG_TYPE_FRAMEDONE)
    {
      printf("Frame done message arrived #%u.\n", 
            lpAppContext->mFrmDoneCnt++);
    } // if frame done arrived
  } // if user pointer is NULL
} // SeqEventCallBack()

//***************************************************************************

#ifndef __STANDALONE__
static void SigintHandler(int)
{
  sStop = true;
} // SigintHandler()

//***************************************************************************

static int32_t SigintSetup()
{
  int32_t lRet = SEQ_LIB_SUCCESS;
  
  // prepare internal signal handler
  struct sigaction lSa;
  memset(&lSa, 0, sizeof(lSa));
  lSa.sa_handler = SigintHandler;
  
  if( sigaction(SIGINT, &lSa, NULL) != 0)
  {
    VDB_LOG_ERROR("Failed to register signal handler.\n");
    lRet = SEQ_LIB_FAILURE;
  } // if signal not registered
  
  return lRet;
} // SigintSetup()

//***************************************************************************
#endif // #ifndef __STANDALONE__



//---------------------------------------2018.9.4-----------------------------------------------



// ���� EAR
double eyeAspectRatio(std::vector<cv::Point>eye)
{
	// EAR = ( ||p6 - p2|| + ||p5-p3|| ) / ( 2 * ||p4 - p1|| )
	double short_axis_A = calcTwoNormIsEuclid(eye[5], eye[1]);
	double short_axis_B = calcTwoNormIsEuclid(eye[4], eye[2]);
	double long_axis_C = calcTwoNormIsEuclid(eye[3], eye[0]);
	double calc_ear = (short_axis_A + short_axis_B) / (2.0 * long_axis_C);
	return calc_ear;
}

// ���� 2 ����
double calcTwoNormIsEuclid(cv::Point p1, cv::Point p2)
{
	double dist;
	dist = sqrt(pow((p2.x - p1.x), 2) + pow((p2.y - p1.y), 2));
	return dist;
}

string doubleTwoRound(double dVal)
{
	char buf_time[10];
	sprintf(buf_time, "%.2f", dVal);
	stringstream ss;
	ss << buf_time;
	return ss.str();
}

//��������
int round_double(double number)
{
	return (number > 0.0) ? floor(number + 0.5) : ceil(number - 0.5);
}

string getCurrentTime()
{
	time_t t = time(NULL);
	char ch[64] = { 0 };
	strftime(ch, sizeof(ch)-1, "%H:%M:%S", localtime(&t));
	return ch;
}

//1. ƣ��һ�Σ���ʾһ�У� ƣ��������ʾ���У���һ�е����ݲ��ܱ䣬�ٴλ��������ӣ���������ʱ������ʾ����һ��û�ˣ�
//��һ�μ�⵽˳���ڵ���������
cv::Mat scrollScreen(int fatigue, cv::Mat& temp)
{
	char fatigue_path[200];



	if (fatigue <= 4)
	{

		if (fatigue == 1)
		{
			if (flag_one == true)
			{
				//sprintf(fatigue_path, fataguePara[0], getCurrentTime().c_str());
				sprintf(fatigue_path, fataguePara[0], getCurrentTime().c_str());
				fa.push_back(fatigue_path);
				flag_one = false;
			}
			cv::putText(temp, fa[0], point_arr[0], CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 255, 0), 2);
		}

		else if (fatigue == 2)
		{
			if (flag_two == true)
			{
				sprintf(fatigue_path, fataguePara[1], getCurrentTime().c_str());
				fa.push_back(fatigue_path);
				flag_two = false;
			}
			for (unsigned int i = 0; i < fa.size(); i++)
			{
				if (i < fa.size() - 1) // 
				{
					cv::putText(temp, fa[i], point_arr[i], CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(125, 125, 125), 2);
				//	FILE *file_i = fopen("jin.txt", "a");
				//	fprintf(file_i, "----------%d\n", i);
				//	fclose(file_i);
				}
				else
				{
					cv::putText(temp, fa[i], point_arr[i], CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 255, 0), 2);
				}
			}
		}

		else if (fatigue == 3)
		{
			if (flag_three == true)
			{
				sprintf(fatigue_path, fataguePara[2], getCurrentTime().c_str());
				fa.push_back(fatigue_path);
				flag_three = false;
			}
			for (unsigned int i = 0; i < fa.size(); i++)
			{
				if (i < fa.size() - 1)
				{
					cv::putText(temp, fa[i], point_arr[i], CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(125, 125, 125), 2);
				}
				else
				{
					cv::putText(temp, fa[i], point_arr[i], CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 255, 0), 2);
				}
			}
		}
		else if (fatigue == 4)
		{
			if (flag_four == true)
			{
				sprintf(fatigue_path, fataguePara[3], getCurrentTime().c_str());
				fa.push_back(fatigue_path);
				flag_four = false;
			}
			for (unsigned int i = 0; i < fa.size(); i++)
			{
				if (i < fa.size() - 1)
				{
					cv::putText(temp, fa[i], point_arr[i], CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(125, 125, 125), 2);
				}
				else
				{
					cv::putText(temp, fa[i], point_arr[i], CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 255, 0), 2);
				}
			}
		}
	}
	else // fatigue >= 5   //Ĭ�ϣ� 0 1 2 3      
	{
		// 0 1 2 3 --- 1 2 3   2 3 0 1 --- 3 0 1
		if (tony_flag != fatigue)
		{
			flag_big = true;
			tmp_display_fatigue = "";
		}
		if (flag_big == true) // 
		{
			for (unsigned int i = 0; i < 4; i++)  // 1 2 3 0  ||  2 3 0 1
			{
				array2[i] = (array2[i] + 1) % 4;
				// fa �� 4 ��ֵ
			}

			cout << " sizeof(array2) = " << sizeof(array2) << endl;
			for (unsigned int i = 0; i < 4 - 1; i++) // 1 2 3 0 ||  2 3 0 1   6
			{
				tmp_display_fatigue += fa[array2[i]];  // tmp = 1+2+3   tmp = 2+3+0
				tmp_display_fatigue += "\n";
			}
			//--------------print-------------------------------
			/*FILE *file_i = fopen("test_fatigue.txt", "a");
			  fprintf(file_i, "%s\n", tmp_display_fatigue.c_str());*/
			//------------------print end-------------------------

			sprintf(fatigue_path, "%s ; You have fatigued !\n", getCurrentTime().c_str());
			tmp_display_fatigue += fatigue_path;

			//cout << "tmp_display_fatigue1 = " << tmp_display_fatigue << endl;
			//sprintf(tmp_str, "%s\n", tmp_display_fatigue);
			//fprintf(file_i, "%s\n", tmp_display_fatigue.c_str());

			//tmp_display_fatigue = tmp_str;
			fa.erase(fa.begin()); // fa = 0, 1, 2, 3 ----> 1 2 3
			fa.push_back(fatigue_path); // 1 2 3 4
			flag_big = false;
			tony_flag = fatigue;
			for (unsigned int i = 0; i < fa.size(); i++)
			{
				cout << "fa[" << i << "] = " << fa[i] << endl;
			}
			cout << "come in : %d " << test_e5++ << endl;
			cout << "f = " << tmp_display_fatigue << endl;
			//cv::waitKey(0);
		}

		//tmp_display_fatigue �ָ�
		//cv::Point point_arr[4] = { cv::Point(25, 100), cv::Point(25, 120), cv::Point(25, 140), cv::Point(25, 160) };
		std::vector<string> chara_string_spli = characterStingSplit(tmp_display_fatigue);
		for (unsigned int i = 0; i < chara_string_spli.size(); i++)
		{
			if (i < chara_string_spli.size() - 1)
			{
				cv::putText(temp, chara_string_spli[i], cv::Point(point_arr[0].x, point_arr[0].y + i * SCREEN_LINE_SEG), CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(125, 125, 125), 2);
			}
			else
			{
				cv::putText(temp, chara_string_spli[i], cv::Point(point_arr[0].x, point_arr[0].y + i * SCREEN_LINE_SEG), CV_FONT_HERSHEY_SIMPLEX, FONT_SIZE_DIS, cv::Scalar(0, 255, 0), 2);
			}
		}

	}

	return temp;
}

//�ַ����ָ�
std::vector<string> characterStingSplit(string pend_string)
{
	std::vector<string>stor_split_string;
	char* dd_p = const_cast<char*>(pend_string.c_str());
	const char * split = "\n";
	char * p;

	p = strtok(dd_p, split);
	while (p != NULL)
	{
		//printf("%s\n", p);
		stor_split_string.push_back(p);
		p = strtok(NULL, split);
	}
	return stor_split_string;
}


//----------------------------2018.9.4---------------------------------------------------------------------













