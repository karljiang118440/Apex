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
    //mCapture.open("./data/airunner/dms/call.avi");
    mCapture.open("./data/airunner/dms/call_red.avi");
   // mCapture.open("./data/airunner/dms/test_video.avi");
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
//	myDMS.Init(FRAME_DMS_RGB, FRAME_DMS_GS, IMG_WIDTH, IMG_HEIGHT);
    myDMS.Init(1, 0, IMG_WIDTH, IMG_HEIGHT);  // add by frank


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
    vsdk::UMat frameUMat[2];

    vsdk::UMat frameUMatInput[2];

    /* Quit signal */    
    volatile sig_atomic_t quit = 0;

    int counter = 0;

    /* Processing loop */
    while (quit != 1) 
    {
        quit = KeyScan();        
#if (DEMO_MODE == CAMERA_MODE)    
//        ImageGrabberPop(frameUMat);
        ImageGrabberPop(frameUMatInput);
       // cv::imwrite("cvframe.jpg",(cv::Mat)frameUMat[FRAME_DMS_RGB]);



    #if 0  // raw opencv cvt ---karl -2020.11.30

        cv::Mat video_frame_frank;
        cv::Mat video_frame_gray_frank;

        video_frame_frank = frameUMatInput[0].getMat (vsdk::ACCESS_RW | OAL_USAGE_CACHED);
        cv::cvtColor(video_frame_frank, video_frame_gray_frank, cv::COLOR_RGB2GRAY);
        frameUMat[1] = frameUMatInput[0];

        //cv::imwrite("video_frame.jpg",video_frame);
        //cv::imwrite("video_frame_gray.jpg",video_frame_gray);
        frameUMat[0] = video_frame_gray_frank.getUMat(cv::ACCESS_READ);


    #endif 




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

        video_frame_frank = frameUMatInput[0].getMat (vsdk::ACCESS_RW | OAL_USAGE_CACHED);


        cv::cvtColor(video_frame_frank, video_frame_gray_frank, cv::COLOR_RGB2GRAY);
        frameUMat[1] = frameUMatInput[0];


	  //------add wpi logo------
	 // cv::Mat imageROI = video_frame_frank(cv::Rect(video_frame_frank.cols - logo.cols, video_frame_frank.rows - logo.rows, logo.cols, logo.rows)); 

	  cv::Mat imageROI = video_frame_frank(cv::Rect(video_frame_frank.cols - logo.cols, video_frame_frank.rows - logo.rows -600, logo.cols, logo.rows)); 


	  cv::addWeighted(imageROI, 0.2, logo, 0.8, 0.0, imageROI);
	  //-----end add wpi logo------

        //cv::imwrite("video_frame.jpg",video_frame);
        //cv::imwrite("video_frame_gray.jpg",video_frame_gray);
        frameUMat[0] = video_frame_gray_frank.getUMat(cv::ACCESS_READ);




    #endif 





    # if 1 // add face recogize by karl -2020.12.02



    





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

        //cv::imwrite("video_frame.jpg",video_frame);
        //cv::imwrite("video_frame_gray.jpg",video_frame_gray);

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
