/*#include <opencv2\opencv.hpp>
#include <cstdlib>

using namespace cv;
using namespace std;

int main(int argc, char* argv[])
{
	Mat img = imread(argv[1], atoi(argv[2]));
	if (img.empty()) return -1;
	namedWindow("Image", WINDOW_AUTOSIZE);
	imshow("Image", img);
	waitKey(0);
	destroyWindow("Image");
	return 0;

    cv::VideoCapture vcap;
    cv::Mat image;

    // This works on a D-Link CDS-932L
    //const std::string videoStreamAddress = "http://<username:password>@<ip_address>/video.cgi?.mjpg";
    const std::string videoStreamAddress = "http://admin:K1_a9_a9_i7@192.168.1.157:8000/video.cgi?.mjpg";

    //open the video stream and make sure it's opened
    if (!vcap.open(videoStreamAddress)) {
        std::cout << "Error opening video stream or file" << std::endl;
        return -1;
    }

    for (;;) {
        if (!vcap.read(image)) {
            std::cout << "No frame" << std::endl;
            cv::waitKey();
        }
        cv::imshow("Output Window", image);
        if (cv::waitKey(1) >= 0) break;
    }
}*/
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include "MvCameraControl.h";

using namespace cv;
using namespace std;

enum CONVERT_TYPE
{
    OpenCV_Mat = 0,    // ch:Mat图像格式 | en:Mat format
    OpenCV_IplImage = 1,    // ch:IplImage图像格式 | en:IplImage format
};

void PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo)
{
    if (NULL == pstMVDevInfo)
    {
        printf("    NULL info.\n\n");
        return;
    }

    // 获取图像数据帧仅支持GigE和U3V设备
    if (MV_GIGE_DEVICE == pstMVDevInfo->nTLayerType)
    {
        int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

        // ch:显示IP和设备名 | en:Print current ip and user defined name
        printf("    IP: %d.%d.%d.%d\n", nIp1, nIp2, nIp3, nIp4);
        printf("    UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
        printf("    Device Model Name: %s\n\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
    }
    else if (MV_USB_DEVICE == pstMVDevInfo->nTLayerType)
    {
        printf("    UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
        printf("    Device Model Name: %s\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);
    }
    else
    {
        printf("    Not support.\n\n");
    }
}

void RGB2BGR(unsigned char* pRgbData, unsigned int nWidth, unsigned int nHeight)
{
    if (NULL == pRgbData)
    {
        return;
    }

    // red和blue数据互换
    for (unsigned int j = 0; j < nHeight; j++)
    {
        for (unsigned int i = 0; i < nWidth; i++)
        {
            unsigned char red = pRgbData[j * (nWidth * 3) + i * 3];
            pRgbData[j * (nWidth * 3) + i * 3] = pRgbData[j * (nWidth * 3) + i * 3 + 2];
            pRgbData[j * (nWidth * 3) + i * 3 + 2] = red;
        }
    }
}

bool Convert2Mat(MV_FRAME_OUT_INFO_EX* pstImageInfo, unsigned char* pData)
{
    printf("Beginning...\n");
    return 0;
    //
}

bool Convert2Ipl(MV_FRAME_OUT_INFO_EX* pstImageInfo, unsigned char* pData)
{
    return 0;
    //
}

int main(int argc, char* argv[])
{
    int nRet = MV_OK;
    void* handle = NULL;
    unsigned char* pData = NULL;

    do
    {
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
        if (MV_OK != nRet)
        {
            printf("Enum Devices fail! nRet [0x%x]/n");
            break;
        }

        if (stDeviceList.nDeviceNum > 0)
        {
            for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++)
            {
                printf("[device %d]:\n", i);
                MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
                if (NULL == pDeviceInfo)
                {
                    break;
                }
                PrintDeviceInfo(pDeviceInfo);
            }
        }
        else
        {
            printf("Find No Devices!");
            break;
        }

        unsigned int nIndex = 0;
        while (1)
        {
            printf("Please Input camera index(0-%d): ", stDeviceList.nDeviceNum - 1);

            if (1 == scanf_s("%d", &nIndex))
            {
                while (getchar() != '\n')
                {
                    ;
                }

                if (nIndex >= 0 && nIndex < stDeviceList.nDeviceNum)
                {
                    if (false == MV_CC_IsDeviceAccessible(stDeviceList.pDeviceInfo[nIndex], MV_ACCESS_Exclusive))
                    {
                        printf("Can't connect\n");
                        continue;
                    }

                    break;
                }
            }
            else
            {
                while (getchar() != '\n')
                {
                    ;
                }
            }
        }

        nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
        if (MV_OK != nRet)
        {
            printf("Create Handle fail! nRet [0x%x]\n", nRet);
            break;
        }

        nRet = MV_CC_OpenDevice(handle);
        if (MV_OK != nRet)
        {
            printf("Open Device fail! nRet [0x%x]", nRet);
            break;
        }

        if (MV_GIGE_DEVICE == stDeviceList.pDeviceInfo[nIndex]->nTLayerType)
        {
            int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
            if (nPacketSize > 0)
            {
                nRet = MV_CC_SetIntValue(handle, "GevSCPSPacketSize", nPacketSize);
                if (MV_OK != nRet)
                {
                    printf("Warning: Set Packet Size fail! nRet [0x%x]!", nRet);
                }
            }
            else
            {
                printf("Warning: Get Packet Size fail! nRet [0x%x]!", nPacketSize);
            }
        }

        nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
        if (MV_OK != nRet)
        {
            printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
            break;
        }

        MVCC_INTVALUE stParam;
        memset(&stParam, 0, sizeof(MVCC_INTVALUE));
        nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
        if (MV_OK != nRet)
        {
            printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
            break;
        }
        unsigned int nPayloadSize = stParam.nCurValue;

        MV_FRAME_OUT_INFO_EX stImageInfo = { 0 };
        memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
        pData = (unsigned char *)malloc(sizeof(unsigned char)* (nPayloadSize));
        if (NULL == pData)
        {
            printf("Alllocate memory failed.\n");
            break;
        }
        memset(pData, 0, sizeof(pData));

        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
            break;
        }

        nRet = MV_CC_GetOneFrameTimeout(handle, pData, nPayloadSize, &stImageInfo, 1000);
        if (MV_OK == nRet)
        {
            printf("Get One Frame: Width[%d], Height[%d], FrameNum[%d]", stImageInfo.nWidth, stImageInfo.nHeight, stImageInfo.nFrameNum);
        }
        else
        {
            printf("Get Frame fail! nRet [0x%x]\n", nRet);
            break;
        }

        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Stop Grabbing fail! nRet [0x%x]\n", nRet);
            break;
        }

        nRet = MV_CC_CloseDevice(handle);
        if (MV_OK != nRet)
        {
            printf("Close Device fail! nRet [0x%x]\n", nRet);
            break;
        }

        printf("\n[0] OpenCV_Mat\n");
        printf("[1] OpenCV_IplImage\n");
        int nFormat = 0;
        while (1)
        {
            printf("Please Input Format to convert: ");

            if (1 == scanf_s("%d", &nFormat))
            {
                if (0 == nFormat || 1 == nFormat)
                {
                    break;
                }
            }
            while (getchar() != '\n')
            {
                ;
            }
        }

        bool bConvertRet = false;
        if (OpenCV_Mat == nFormat)
        {
            bConvertRet = Convert2Mat(&stImageInfo, pData);
        }
        else if(OpenCV_IplImage == nFormat)
        {
            bConvertRet = Convert2Ipl(&stImageInfo, pData);
        }

        if (bConvertRet)
        {
            printf("OpenCV format convert finished.\n");
        }
        else
        {
            printf("OpenCV format convert failed.\n");
        }
    } while (0);

    if (handle)
    {
        MV_CC_DestroyHandle(handle);
        handle == NULL;
    }

    //******************************************************************************

    printf("\n******************\nEverything is OK\n***********************\n");
    
    //*******************************************************************************

    /*VideoCapture cap; //

    //cap.open("http://admin:K1_a9_a9_i7@192.168.1.157:8000/video?x.mjpeg");
    //cap.open("http://admin:admin@192.168.1.172:8899/video?x.mjpeg");
    //cap.open("http://admin:admin@192.168.1.140/video?x.mjpeg");
    cap.open("cam.mp4");
    if (!cap.isOpened())  // if not success, exit program
    {
        cout << "Cannot open the video cam" << endl;
        return -1;
    }

    double dWidth = cap.get(CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
    double dHeight = cap.get(CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video

    cout << "Frame size : " << dWidth << " x " << dHeight << endl;

    namedWindow("MyVideo", WINDOW_AUTOSIZE); //create a window called "MyVideo"
    namedWindow("MyNegativeVideo", WINDOW_AUTOSIZE);

    while (1)
    {
        Mat frame;
        Mat contours;

        bool bSuccess = cap.read(frame); // read a new frame from video

        if (!bSuccess) //if not success, break loop
        {
            cout << "Cannot read a frame from video stream" << endl;
            break;
        }

        flip(frame, frame, 1);
        imshow("MyVideo", frame); //show the frame in "MyVideo" window

        Canny(frame, contours, 500, 1000, 5, true);
        imshow("MyNegativeVideo", contours);

        if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
        {
            cout << "esc key is pressed by user" << endl;
            break;
        }
    }
    return 0;*/

    Mat img = imread("Schluse.jpg", 1);
    if (img.empty()) return -1;
    namedWindow("Image", WINDOW_AUTOSIZE);
    imshow("Image", img);
    waitKey(0);
    destroyWindow("Image");
    return 0;
}