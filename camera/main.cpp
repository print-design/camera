#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <thread>
#include <Windows.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <regex>
#include "MvCameraControl.h";
#include "usblister.h";
#include "comportslister.h";

using namespace cv;
using namespace std;

atomic<bool> brak = false;
atomic<bool> stop = false;
atomic<bool> switchedOff = false;
atomic<int> switchOffRectTop = 0;
atomic<int> switchOffRectLeft = 0;
atomic<int> switchOffRectWidth = 0;
atomic<int> switchOffRectHeight = 0;

atomic<int> fragmentX = 200;
atomic<int> fragmentY = 200;
atomic<int> fragmentWidth = 600;
atomic<int> fragmentHeight = 600;

void StopSignal(int event, int x, int y, int flags, void* userdata)
{
    if (x >= switchOffRectLeft && x <= switchOffRectLeft + switchOffRectWidth && y >= switchOffRectTop && y <= switchOffRectTop + switchOffRectHeight)
    {
        if (event == EVENT_LBUTTONDOWN)
        {
            switchedOff = true;
        }
    }
}

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

void Signal(string port_name)
{
    while (true)
    {
        if (brak && !switchedOff)
        {
            HANDLE port;
            port = CreateFileA(port_name.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (port == INVALID_HANDLE_VALUE)
            {
                cout << "Error on opening COM-port." << endl;
                ExitProcess(1);
            }
            else
            {
                cout << "COM-port opened." << endl;

                while (brak && !switchedOff)
                {
                    DWORD dwBytesWritten;
                    int buffer[4];
                    int* s;
                    buffer[0] = 1;
                    buffer[1] = 2;
                    buffer[2] = 3;
                    buffer[3] = 4;
                    s = &buffer[0];
                    WriteFile(port, buffer, 3, &dwBytesWritten, NULL);

                    if (stop)
                    {
                        break;
                    }
                }
            }
            CloseHandle(port);
        }

        if (stop)
        {
            cout << "stop" << endl;
            break;
        }
    }
}

void ObserveImage(string port_name, void* handle)
{
    int nRet = MV_OK;
    bool showOriginal = false;
    
    Mat fragment;
    Mat original;
    bool hasFragment = false;
    Mat result;

    int textX = 300;
    int textY = 500;

    int method = TM_CCOEFF_NORMED;
    MVCC_INTVALUE stParam;

    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
    if (MV_OK != nRet)
    {
        printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
        return;
    }
    unsigned int nPayloadSize = stParam.nCurValue;
    unsigned char* pData = NULL;

    MV_FRAME_OUT_INFO_EX stImageInfo = { 0 };
    memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
    pData = (unsigned char*)malloc(sizeof(unsigned char) * (nPayloadSize));
    if (NULL == pData)
    {
        printf("Allocate memory failed.\n");
        return;
    }
    memset(pData, 0, sizeof(pData));

    nRet = MV_CC_StartGrabbing(handle);
    if (MV_OK != nRet)
    {
        printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
        return;
    }

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
        RECT desktop;
        const HWND hDesktop = GetDesktopWindow();
        GetWindowRect(hDesktop, &desktop);
        int desktopWidth = desktop.right;
        int desktopHeight = desktop.bottom;
        namedWindow("Final", WINDOW_NORMAL);

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
                    if (!hasFragment && fragmentHeight > 0 && fragmentWidth > 0)
                    {
                        fragment = Mat(fragmentHeight, fragmentWidth, CV_8UC3);
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
                    Mat matFinal(desktopHeight, desktopWidth, CV_8UC3);

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

                        rectangle(rgbImage, maxLoc, Point(maxLoc.x + fragment.cols, maxLoc.y + fragment.rows), Scalar(0, 255, 9), 2);

                        Mat resizedRgbImage;
                        int rgbImageWidth = desktopWidth / 2;
                        int rgbImageHeight = desktopWidth * rgbImage.rows / rgbImage.cols / 2;
                        resize(rgbImage, resizedRgbImage, Size(rgbImageWidth, rgbImageHeight));
                        resizedRgbImage.copyTo(matFinal(Rect(0, 0, rgbImageWidth, rgbImageHeight)));

                        textY = resizedRgbImage.rows + 30;

                        Mat resizedOriginalCrop;
                        Mat resizedCurrentCrop;

                        if (showOriginal)
                        {
                            int originalCropWidth = desktopWidth / 2;
                            int originalCropHeight = desktopWidth * originalCrop.rows / originalCrop.cols / 2;
                            resize(originalCrop, resizedOriginalCrop, Size(originalCropWidth, originalCropHeight));
                            resizedOriginalCrop.copyTo(matFinal(Rect(rgbImageWidth, 0, originalCropWidth, originalCropHeight)));
                        }
                        else
                        {
                            int currentCropWidth = desktopWidth / 2;
                            int currentCropHeight = desktopWidth * currentCrop.rows / currentCrop.cols / 2;
                            resize(currentCrop, resizedCurrentCrop, Size(currentCropWidth, currentCropHeight));
                            resizedCurrentCrop.copyTo(matFinal(Rect(rgbImageWidth, 0, currentCropWidth, currentCropHeight)));
                        }
                        showOriginal = !showOriginal;

                        if (norm(currentCrop, originalCrop) < 50000)
                        {
                            putText(matFinal, "OK", Point(textX, textY), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 0));
                            switchedOff = false;
                            brak = false;
                        }
                        else
                        {
                            putText(matFinal, "BRAK", Point(textX, textY), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                            if (!switchedOff)
                            {
                                brak = true;
                            }

                            if (brak && !switchedOff)
                            {
                                switchOffRectLeft = textX + 200;
                                switchOffRectTop = textY - 30;
                                switchOffRectWidth = 200;
                                switchOffRectHeight = 40;
                                Rect switchOffRect(switchOffRectLeft, switchOffRectTop, switchOffRectWidth, switchOffRectHeight);
                                rectangle(matFinal, switchOffRect, Scalar(0, 0, 255), -1);
                                putText(matFinal, "Stop signal", Point(textX + 210, textY), FONT_HERSHEY_DUPLEX, 1, Scalar(0, 255, 9));
                                setMouseCallback("Final", StopSignal, NULL);
                            }
                        }

                        cv::String text = cv::format("DeltaX = %i, DeltaY = %i", DeltaX, DeltaY);
                        putText(matFinal, text, Point(textX, textY + 40), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 0));

                        imshow("Final", matFinal);
                        rgbImage.release();
                        originalCrop.release();
                        currentCrop.release();
                        resizedRgbImage.release();
                        resizedOriginalCrop.release();
                        resizedCurrentCrop.release();
                        matFinal.release();

                        if (waitKey(30) == 27)
                        {
                            stop = true;
                            break;
                        }
                    }
                    catch (std::exception stde)
                    {
                        putText(matFinal, cv::format("std exception: %s", stde.what()), Point(textX, textY + 80), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                    }
                    catch (cv::Exception cve)
                    {
                        putText(matFinal, cv::format("cv exception: %s - %s", cve.err, cve.msg), Point(textX, textY + 80), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 255));
                    }
                }
            }
        }

        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Stop Grabbing fail! nRet [0x%x]\n", nRet);
            return;
        }

        nRet = MV_CC_CloseDevice(handle);
        if (MV_OK != nRet)
        {
            printf("Close Device fail! nRet [0x%x]\n", nRet);
            return;
        }

        if (handle)
        {
            MV_CC_DestroyHandle(handle);
            handle == NULL;
        }
    }
}

int main(int argc, char* argv[])
{
    string target_devdesc = "USB-SERIAL CH340";
    string target_devdesc1 = "USB-Enhanced-SERIAL CH343";
    string target_mfg = "wch.cn";
    string target_enumerator_name = "USB";
    string port_name = "";

    // Просматриваем COM-порты
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    SP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData;
    char dev_name[1024];
    char dev_desc[1024];
    char dev_mfg[1024];
    WCHAR szBuffer[400];

    const regex com_regex(R"(COM\d+)");
    smatch m;

    hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        cout << "No COM ports found." << endl;
    }

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    cout << "Device friendly names: " << endl;

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
    {
        if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData, SPDRP_FRIENDLYNAME, NULL, (UCHAR*)dev_name, sizeof(dev_name), NULL))
        {
            cout << " -- " << dev_name << endl;
        }
    }

    cout << "Device descriptions: " << endl;

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
    {
        if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC, NULL, (UCHAR*)dev_desc, sizeof(dev_desc), NULL))
        {
            cout << " -- " << dev_desc << endl;

            if (((new string(dev_desc))->compare(target_devdesc) == 0 || ((new string(dev_desc))->compare(target_devdesc1) == 0))
                && SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData, SPDRP_FRIENDLYNAME, NULL, (UCHAR*)dev_name, sizeof(dev_name), NULL))
            {
                string str_dev_name(dev_name);

                if (regex_search(str_dev_name, m, com_regex) && m.length() > 0)
                {
                    port_name = m[0];
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    // Если нет нужного порта, выводим сообщение и останавливаем программу.
    if (port_name.length() == 0)
    {
        cout << "Find no suitable COM-ports." << endl;
        return 0;
    }

    cout << "Port name: " << port_name << endl;

    // Просматриваем камеры
    int nRet = MV_OK;
    void* handle = NULL;
    

    // Камеры
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

    nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
    if (MV_OK != nRet)
    {
        printf("Enum Devices fail! nRet [0x%x]\n", nRet);
        return 0;
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
        printf("Find No Devices!\n");
        return 0;
    }

    unsigned int nIndex = 0;

    nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
    if (MV_OK != nRet)
    {
        printf("Create Handle fail! nRet [0x%x]\n", nRet);
        return 0;
    }

    nRet = MV_CC_OpenDevice(handle);
    if (MV_OK != nRet)
    {
        printf("Open Device fail! nRet [0x%x]", nRet);
        MV_CC_CloseDevice(handle);
        return 0;
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
    //nRet = MV_CC_SetEnumValue(handle, "TriggerMode", MV_TRIGGER_MODE_ON);
    if (MV_OK != nRet)
    {
        printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
        return 0;
    }

    /*nRet = MV_CC_SetEnumValue(handle, "PixelFormat", PixelType_Gvsp_BayerRG8);
    if (MV_OK != nRet)
    {
        printf("Set Pixel Format fail! nRet [0x%x]\n", nRet);
        MV_CC_CloseDevice(handle);
        break;
    }*/

    // Если всё в порядке, то запускаем просмотр

    thread threadObserveImage = thread(ObserveImage, port_name, handle);
    thread threadSignal = thread(Signal, port_name);
    threadObserveImage.join();
    threadSignal.join();

    return 1;
}