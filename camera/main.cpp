#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include "MvCameraControl.h";

using namespace cv;
using namespace std;

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

int main(int argc, char* argv[])
{
    int nRet = MV_OK;
    void* handle = NULL;
    unsigned char* pData = NULL;
    bool showOriginal = false;

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
            MV_CC_CloseDevice(handle);
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

        nRet = MV_CC_SetEnumValue(handle, "TriggerMode", MV_TRIGGER_MODE_OFF);
        if (MV_OK != nRet)
        {
            printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
            break;
        }

        /*nRet = MV_CC_SetEnumValue(handle, "PixelFormat", PixelType_Gvsp_BayerRG8);
        if (MV_OK != nRet)
        {
            printf("Set Pixel Format fail! nRet [0x%x]\n", nRet);
            MV_CC_CloseDevice(handle);
            break;
        }*/

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
        pData = (unsigned char*)malloc(sizeof(unsigned char) * (nPayloadSize));
        if (NULL == pData)
        {
            printf("Allocate memory failed.\n");
            break;
        }
        memset(pData, 0, sizeof(pData));

        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
            break;
        }

        int fragmentX = 200;
        int fragmentY = 200;
        int fragmentWidth = 600;
        int fragmentHeight = 600;
        Mat fragment(fragmentHeight, fragmentWidth, CV_8UC3);
        Mat original;
        bool hasFragment = false;
        Mat result;
        int method = TM_CCOEFF_NORMED;

        nRet = MV_CC_GetOneFrameTimeout(handle, pData, nPayloadSize, &stImageInfo, 1000);
        if (MV_OK == nRet)
        {
            printf("Get First Frame: Width[%d], Height[%d], FrameNum[%d]\n", stImageInfo.nWidth, stImageInfo.nHeight, stImageInfo.nFrameNum);
        }
        else
        {
            printf("Get First Frame fail! nRet [0x%x]\n", nRet);
        }

        if (NULL == pData)
        {
            printf("NULL info or data.\n");
        }
        else
        {
            namedWindow("Image", WINDOW_AUTOSIZE);
            namedWindow("Compare", WINDOW_AUTOSIZE);

            while (1)
            {
                nRet = MV_CC_GetOneFrameTimeout(handle, pData, nPayloadSize, &stImageInfo, 1000);
                if (MV_OK == nRet)
                {
                    printf("Get One Frame: Width[%d], Height[%d], FrameNum[%d]\n", stImageInfo.nWidth, stImageInfo.nHeight, stImageInfo.nFrameNum);
                }
                else
                {
                    printf("Get Frame fail! nRet [0x%x]\n", nRet);
                    break;
                }

                if (NULL == pData)
                {
                    printf("NULL info or data.\n");
                }
                else
                {
                    Mat srcImage = Mat(stImageInfo.nHeight, stImageInfo.nWidth, CV_8UC1, pData);
                    Mat rgbImage = Mat(stImageInfo.nHeight, stImageInfo.nWidth, CV_8UC3);
                    cvtColor(srcImage, rgbImage, COLOR_BayerRG2RGB);

                    if (NULL == rgbImage.data)
                    {
                        printf("Create Mat failed.\n");
                    }
                    else
                    {
                        if (!hasFragment)
                        {
                            rgbImage.copyTo(original);
                            fragment = rgbImage(Rect(fragmentX, fragmentY, fragmentWidth, fragmentHeight)).clone();
                            result.create(rgbImage.rows - fragment.rows + 1, rgbImage.cols - fragment.cols + 1, CV_32FC1);
                            hasFragment = true;
                        }

                        matchTemplate(rgbImage, fragment, result, method);
                        double minVal;
                        double maxVal;
                        Point minLoc;
                        Point maxLoc;
                        Point matchLoc;
                        minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

                        int DeltaX = maxLoc.x - fragmentX;
                        int DeltaY = maxLoc.y - fragmentY;

                        Mat currentCrop;
                        Mat originalCrop;

                        try
                        {
                            if (DeltaX < 0 && DeltaY < 0)
                            {
                                currentCrop = rgbImage(Rect(0, 0, stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight - abs(DeltaY))).clone();
                                originalCrop = original(Rect(abs(DeltaX), abs(DeltaY), stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight - abs(DeltaY))).clone();
                            }
                            else if (DeltaX < 0 && DeltaY > 0)
                            {
                                currentCrop = rgbImage(Rect(0, abs(DeltaY), stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight - abs(DeltaY))).clone();
                                originalCrop = original(Rect(abs(DeltaX), 0, stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight - abs(DeltaY))).clone();
                            }
                            else if (DeltaX < 0 && DeltaY == 0)
                            {
                                currentCrop = rgbImage(Rect(0, 0, stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight)).clone();
                                originalCrop = original(Rect(abs(DeltaX), 0, stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight)).clone();
                            }
                            else if (DeltaX > 0 && DeltaY < 0)
                            {
                                currentCrop = rgbImage(Rect(abs(DeltaX), 0, stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight - abs(DeltaY))).clone();
                                originalCrop = original(Rect(0, abs(DeltaY), stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight - abs(DeltaY))).clone();
                            }
                            else if (DeltaX > 0 && DeltaY > 0)
                            {
                                currentCrop = rgbImage(Rect(abs(DeltaX), abs(DeltaY), stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight - abs(DeltaY))).clone();
                                originalCrop = original(Rect(0, 0, stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight - abs(DeltaY))).clone();
                            }
                            else if (DeltaX > 0 && DeltaY == 0)
                            {
                                currentCrop = rgbImage(Rect(abs(DeltaX), 0, stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight)).clone();
                                originalCrop = original(Rect(0, 0, stImageInfo.nWidth - abs(DeltaX), stImageInfo.nHeight)).clone();
                            }
                            else if (DeltaX == 0 && DeltaY < 0)
                            {
                                currentCrop = rgbImage(Rect(0, 0, stImageInfo.nWidth, stImageInfo.nHeight - abs(DeltaY))).clone();
                                originalCrop = original(Rect(0, abs(DeltaY), stImageInfo.nWidth, stImageInfo.nHeight - abs(DeltaY))).clone();
                            }
                            else if (DeltaX == 0 && DeltaY > 0)
                            {
                                currentCrop = rgbImage(Rect(0, abs(DeltaY), stImageInfo.nWidth, stImageInfo.nHeight - abs(DeltaY))).clone();
                                originalCrop = original(Rect(0, 0, stImageInfo.nWidth, stImageInfo.nHeight - abs(DeltaY))).clone();
                            }
                            else if (DeltaX == 0 && DeltaY == 0)
                            {
                                currentCrop = rgbImage(Rect(0, 0, stImageInfo.nWidth, stImageInfo.nHeight)).clone();
                                originalCrop = original(Rect(0, 0, stImageInfo.nWidth, stImageInfo.nHeight)).clone();
                            }

                            Point pt1(fragmentX, fragmentY);
                            Point pt2(fragmentX + fragmentWidth, fragmentY + fragmentHeight);
                            rectangle(rgbImage, maxLoc, Point(maxLoc.x + fragment.cols, maxLoc.y + fragment.rows), Scalar(0, 255, 9), 2);
                            cv::String text = cv::format("DeltaX = %i, DeltaY = %i", DeltaX, DeltaY);
                            putText(rgbImage, text, Point(maxLoc.x, maxLoc.y + fragment.rows + 100), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 9));

                            if (norm(currentCrop, originalCrop) < 50000)
                            {
                                putText(rgbImage, "OK", Point(maxLoc.x, maxLoc.y + fragment.rows + 50), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 9));
                            }
                            else
                            {
                                putText(rgbImage, "BRAK", Point(maxLoc.x, maxLoc.y + fragment.rows + 50), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                            }

                            cv::String originalSize = cv::format("X1 = %i, Y1 = %i", originalCrop.cols, originalCrop.rows);
                            putText(rgbImage, originalSize, Point(maxLoc.x, maxLoc.y + fragment.rows + 150), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 9));
                            cv::String currentSize = cv::format("X2 = %i, Y2 = %i", currentCrop.cols, currentCrop.rows);
                            putText(rgbImage, currentSize, Point(maxLoc.x, maxLoc.y + fragment.rows + 200), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 9));
                        }
                        catch (std::exception stde)
                        {
                            putText(rgbImage, cv::format("std exception: %s", stde.what()), Point(maxLoc.x, maxLoc.y + fragment.rows + 250), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                        }
                        catch (cv::Exception cve)
                        {
                            putText(rgbImage, cv::format("cv exception: %s - %s", cve.err, cve.msg), Point(maxLoc.x, maxLoc.y + fragment.rows + 250), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                        }

                        imshow("Image", rgbImage);
                        rgbImage.release();

                        if (showOriginal)
                        {
                            imshow("Compare", originalCrop);
                            originalCrop.release();
                        }
                        else
                        {
                            imshow("Compare", currentCrop);
                            currentCrop.release();
                        }
                        showOriginal = !showOriginal;

                        if (waitKey(30) == 27)
                        {
                            break;
                        }
                    }
                }
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

            if (handle)
            {
                MV_CC_DestroyHandle(handle);
                handle == NULL;
            }
        }
    } while (0);
}