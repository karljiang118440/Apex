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

#include "config.hpp"               //Public header files & global control settings should be here

#include "DMS.h"                    //DMS: all DMS related functions
#include "common_time_measure.h"    //Timers for profiling
#include "apex.h"
#include "KeyConnect.hpp"           //Utils: keyboard interaction
#if (DEMO_MODE == CAMERA_MODE)
#include "ImageGrabber.hpp"
#endif //#if (DEMO_MODE == CAMERA_MODE)
#include "XMLReader.hpp"            //Utils: reading parameters from XML
#include "Display.hpp"              //Display: display

#if DEBUG_SEG_FAULT
#include <signal.h>
#include <execinfo.h>
#endif  //#if DEBUG_SEG_FAULT

#if (DEMO_MODE == VIDEO_MODE)
#define FRAME_CHANNEL 2
#define FRAME_DMS_RGB 1
#define FRAME_DMS_GS 0
#endif // #if (DEMO_MODE == VIDEO_MODE)

using namespace std;



/*

add face recoginze


*/ 
extern AppContext s_lContext;


//  add facerecog declear

#include "opencv2/opencv.hpp"
#include "./FaceRecog/face_engine.h"

int TestRecognize(int argc, char* argv[]); 
int TestRecognize_camera(cv::Mat frame_input);

//********************************************







//#define SAVE_VIDEO

#if DEBUG_SEG_FAULT
void dump(int signo)
{
    void *buffer[30] = {0};
    size_t size;
    char **strings = NULL;
    size_t i = 0;

    size = backtrace(buffer, 30);
    fprintf(stdout, "Obtained %zd stack frames.\n", size);
    strings = backtrace_symbols(buffer, size);

    if(strings == NULL)
    {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < size; i++)
    {
        fprintf(stdout, "%s\n", strings[i]);
    }
    free(strings);
    strings = NULL;
    exit(0);
}
#endif  //#if DEBUG_SEG_FAULT

/***************************************************************************************
***************************************************************************************/
/* Program entry. */
int main(int argc, char** argv)
{
#if DEBUG_SEG_FAULT
    if(signal(SIGSEGV, dump) == SIG_ERR) 
    {
         perror("can't catch SIGSEGV");
    }
    //command: aarch64-linux-gnu-objdump -S xxx.elf >xxx.s
#endif  //#if DEBUG_SEG_FAULT

    /* Key scan */   
    KeyInit();

    /* Read settings */
    XMLParameters params;
    if(argc < 2)
    {
        ReadXMLSettings(&params);
    }
    else
    {
        ReadXMLSettings(&params, argv[1]);
    }

    /* ISP perpare */
#if (DEMO_MODE == CAMERA_MODE)
    ImageGrabberInit();
#endif // #if (DEMO_MODE == CAMERA_MODE)

#if (DEMO_MODE == VIDEO_MODE)
    cv::VideoCapture mCapture; /* open video */
    mCapture.open("./data/airunner/dms/output.mp4");
    if(!mCapture.isOpened()) // if not success, exit program
    {
      printf("Cannot open the video file: \n");
      return -1;
    } // if video open failed
    int currentFrame = 0;
    int totalFrameNumber = mCapture.get(CV_CAP_PROP_FRAME_COUNT);
    mCapture.set(1, currentFrame);
    printf("total frames = %d", totalFrameNumber);
#endif // #if (DEMO_MODE == VIDEO_MODE)

    /* APEX init */
    APEX_Init();  

    /* Display init */
    ssv::Display myDisplay;
    myDisplay.Init(&params);

    /* Render init */
    Render myRender;
    myRender.Init(&params);

	DMS myDMS;
	myDMS.Init(FRAME_DMS_RGB, FRAME_DMS_GS, IMG_WIDTH, IMG_HEIGHT);

#ifdef SAVE_VIDEO
    cv::VideoWriter mVideoWriter;
    int fps = 30;
    mVideoWriter.open("output_video.avi", CV_FOURCC('D','I','V','X'), fps, cv::Size(IMG_WIDTH, IMG_HEIGHT));
#endif // #ifdef SAVE_VIDEO

    /* Viriables for profiling and quiting */
    double sumAlgoTicks = 0.0f;
    double sumDsplayTicks = 0.0f;
    double algoFps = 0.0f;
    double displayFps = 0.0f;

    /* Video streams */
    vsdk::UMat frameUMat[FRAME_CHANNEL];

    /* Quit signal */    
    volatile sig_atomic_t quit = 0;

    int counter = 0;


    /*
    facerecognize 

    */

    int counter1 = 0;

    //****************************





    /* Processing loop */
    while (quit != 1) 
    {
        quit = KeyScan();        
#if (DEMO_MODE == CAMERA_MODE)    
        ImageGrabberPop(frameUMat);






/*face recognize 

1. vsdk::mat - > cv::mat

2. add log 

*/

    #if 1 // add wpi_logo  by karl 



  // read wpi_logo
  cv::Mat logo = cv::imread("wpi_text.jpg");
  if (!logo.data)
  {
	  std::cout << "wpi log load filed" << std::endl;
	 // return -1;
  }

        cv::Mat video_frame_frank;
        cv::Mat video_frame_gray_frank;

        video_frame_frank = frameUMat[0].getMat (vsdk::ACCESS_RW | OAL_USAGE_CACHED);

        cv::cvtColor(video_frame_frank, video_frame_gray_frank, cv::COLOR_RGB2GRAY);


	  //------add wpi logo------
	 // cv::Mat imageROI = video_frame_frank(cv::Rect(video_frame_frank.cols - logo.cols, video_frame_frank.rows - logo.rows, logo.cols, logo.rows)); 

	    cv::Mat imageROI = video_frame_frank(cv::Rect(video_frame_frank.cols - logo.cols, video_frame_frank.rows - logo.rows -600, logo.cols, logo.rows)); 


	    cv::addWeighted(imageROI, 0.2, logo, 0.8, 0.0, imageROI);

	    //-----end add wpi logo------

        cv::imwrite("video_frame_frank.jpg",video_frame_frank);
        //cv::imwrite("video_frame_gray.jpg",video_frame_gray);
        //frameUMat[0] = video_frame_gray_frank.getUMat(cv::ACCESS_READ);


    #endif 











#endif //#if (DEMO_MODE == CAMERA_MODE) 

#if (DEMO_MODE == VIDEO_MODE)
        cv::Mat video_frame;
        cv::Mat video_frame_gray;
        if(!mCapture.read(video_frame))
        {
            printf("\nread video frame failed!\n"); /* capture opencv frame */
            mCapture.set(1, 0);
            continue;
        }
        frameUMat[FRAME_DMS_RGB] = video_frame.getUMat(cv::ACCESS_READ);
        cv::cvtColor(video_frame, video_frame_gray, cv::COLOR_RGB2GRAY);
        frameUMat[FRAME_DMS_GS] = video_frame_gray.getUMat(cv::ACCESS_READ);
       
#endif //#if (DEMO_MODE == VIDEO_MODE) 

#ifdef SAVE_VIDEO
        std::cout << "start recording" << std::endl;
        cv::Mat frameRGB = frameUMat[FRAME_DMS_RGB].getMat(vsdk::ACCESS_RW | OAL_USAGE_CACHED);
        mVideoWriter.write(frameRGB);
#else
        double dmsAlgoStart = FSL_Ticks();
        myDMS.Run(frameUMat, quit);
        double dmsAlgoEnd = FSL_Ticks();

        myRender.Run(frameUMat, myDMS);
        myDisplay.Run();
        myDMS.Sync();

        // Calculate the algo FPS except render and display time
        sumAlgoTicks += FSL_TicksToSeconds(dmsAlgoEnd - dmsAlgoStart);
        
        counter ++;
        if (counter == 10)
        {
            algoFps = 1.0f / (sumAlgoTicks / 10);
            sumAlgoTicks = 0.0f;
            counter = 0;
        }
        sprintf(myRender.fpsInfo, "Algo FPS: %.1f", algoFps);





        counter1 ++;
        if(counter1 == 100)
        {
          //TestRecognize(argc, argv);
          TestRecognize_camera(video_frame_frank);
          //counter1 = 0;


        }

        printf("counter1 = %d \n",counter1);



#endif  //#ifdef SAVE_VIDEO 

#if (DEMO_MODE == CAMERA_MODE)
        ImageGrabberPush();
#endif //#if (DEMO_MODE == CAMERA_MODE)
    }

    printf("Quit program, waiting for release resource...\n");

/* Release resources */
#ifdef SAVE_VIDEO
    mVideoWriter.release();
#endif // #ifdef SAVE_VIDEO
#if (DEMO_MODE == CAMERA_MODE)
    ImageGrabberClean();
#endif //#if (DEMO_MODE == CAMERA_MODE)
#if (DEMO_MODE == VIDEO_MODE)
    mCapture.release();
#endif //#if (DEMO_MODE == VIDEO_MODE)
    KeyCleanup();

    myDMS.Quit();

    return 0;
}





int TestRecognize(int argc, char* argv[]) {
	// cv::Mat img_src = cv::imread("./images/4.jpg");
	const char* root_path = "./data/airunner/FaceRecog/models";

	double start = static_cast<double>(cv::getTickCount());
	FaceEngine face_engine;
	face_engine.LoadModel(root_path);
	std::vector<FaceInfo> faces;
	// face_engine.Detect(img_src, &faces);


	cv::Mat face1 = cv::imread("./data/airunner/FaceRecog/images/karl.jpg");
	//cv::Mat face2 = cv::imread("../images/stark.png");
	cv::Mat face2 = cv::imread("./data/airunner/FaceRecog/images/karl2.jpg");

	std::vector<float> feature1, feature2;
	face_engine.ExtractFeature(face1, &feature1);
	face_engine.ExtractFeature(face2, &feature2);
	float sim = CalculSimilarity(feature1, feature2);

	double end = static_cast<double>(cv::getTickCount());
	double time_cost = (end - start) / cv::getTickFrequency() * 1000;
	std::cout << "time cost: " << time_cost << "ms" << std::endl;

	// for (int i = 0; i < static_cast<int>(faces.size()); ++i) {
	// 	cv::Rect face = faces.at(i).face_;
	// 	cv::rectangle(img_src, face, cv::Scalar(0, 255, 0), 2);		
	// }

	std::cout << "similarity is: " << sim << std::endl;

    printf("face recognize finised \n");

    // add label to frames ,threshold is 0.75

    // if(sim >= 0.75)
    
    //     return 1;


}



int TestRecognize_camera(cv::Mat frame_input) {
	// cv::Mat img_src = cv::imread("./images/4.jpg");
	const char* root_path = "./data/airunner/FaceRecog/models";

	double start = static_cast<double>(cv::getTickCount());
	FaceEngine face_engine;
	face_engine.LoadModel(root_path);
	std::vector<FaceInfo> faces;
	face_engine.Detect(frame_input, &faces);


	cv::Mat face1 = cv::imread("./data/airunner/FaceRecog/images/karl_gray.jpg");
	//cv::Mat face2 = cv::imread("../images/stark.png");
	// cv::Mat face2 = cv::imread("./data/airunner/FaceRecog/images/karl2.jpg");
    cv::Mat face2 = frame_input(faces[0].face_).clone();

	std::vector<float> feature1, feature2;
	face_engine.ExtractFeature(face1, &feature1);
	face_engine.ExtractFeature(face2, &feature2);
	float sim = CalculSimilarity(feature1, feature2);

	double end = static_cast<double>(cv::getTickCount());
	double time_cost = (end - start) / cv::getTickFrequency() * 1000;
	std::cout << "time cost: " << time_cost << "ms" << std::endl;

	// for (int i = 0; i < static_cast<int>(faces.size()); ++i) {
	// 	cv::Rect face = faces.at(i).face_;
	// 	cv::rectangle(img_src, face, cv::Scalar(0, 255, 0), 2);		
	// }

	std::cout << "similarity is: " << sim << std::endl;

    printf("face recognize finised \n");

    // add label to frames ,threshold is 0.75

    // if(sim >= 0.75)
    
    //     return 1;


}
