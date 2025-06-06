#include <iostream>
#include <Windows.h>
#include "usblister.h"

using namespace std;

void List()
{
    printf("USB device lister.\n");
    UINT nDevices = 0;
    GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST));

    if (nDevices < 1)
    {
        printf("ERR: 0 Devices");
    }
    else
    {
        cout << nDevices << " DEVICES: " << endl;

        PRAWINPUTDEVICELIST pRawInputDeviceList;

        pRawInputDeviceList = new RAWINPUTDEVICELIST[sizeof(RAWINPUTDEVICELIST) * nDevices];

        if (pRawInputDeviceList == NULL)
        {
            printf("ERR: Could not allocate memory for Device List.\n");
        }

        int nResult;

        if (pRawInputDeviceList != NULL)
        {
            nResult = GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST));
        }

        if (nResult < 0)
        {
            delete[] pRawInputDeviceList;
            printf("ERR: Could not get device list.\n");
        }

        for (UINT i = 0; i < nDevices; i++)
        {
            UINT nBufferSize = 0;
            nResult = GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, NULL, &nBufferSize);

            if (nResult < 0)
            {
                printf("ERR: Unable to get Device Name character count... Moving to next device.\n");
                //continue;
            }

            WCHAR* wcDeviceName = new WCHAR[nBufferSize + 1];

            if (wcDeviceName == NULL)
            {
                printf("ERR: Unable to allocate memory for Device Name... Moving to next device.");
                //continue;
            }

            nResult = GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, wcDeviceName, &nBufferSize);

            if (nResult < 0)
            {
                printf("ERR: Unable to get Device Name... MOving to next device.\n");
                delete[] wcDeviceName;
                //continue;
            }

            RID_DEVICE_INFO rdiDeviceInfo;
            rdiDeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
            nBufferSize = rdiDeviceInfo.cbSize;

            nResult = GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICEINFO, &rdiDeviceInfo, &nBufferSize);

            if (nResult < 0)
            {
                printf("ERR: Unable to read Device Info... Move to next device.\n");
                //continue;
            }

            if (rdiDeviceInfo.dwType == RIM_TYPEMOUSE)
            {
                cout << endl << "Displaying device " << i + 1 << " information. (MOUSE)" << endl;
                wcout << L"Device Name: " << wcDeviceName << endl;
                cout << "Mouse ID: " << rdiDeviceInfo.mouse.dwId << endl;
                cout << "Mouse buttons: " << rdiDeviceInfo.mouse.dwNumberOfButtons << endl;
                cout << "Mouse sample rate (Data Points): " << rdiDeviceInfo.mouse.dwSampleRate << endl;
                if (rdiDeviceInfo.mouse.fHasHorizontalWheel)
                {
                    cout << "Mouse has horizontal wheel" << endl;
                }
                else
                {
                    cout << "Mouse does not have horizontal wheel" << endl;
                }
            }
            else if (rdiDeviceInfo.dwType == RIM_TYPEKEYBOARD)
            {
                cout << endl << "Displaying device " << i + 1 << " information. (KEYBOARD)" << endl;
                wcout << L"Device Name: " << wcDeviceName << endl;
                cout << "Keyboard mode: " << rdiDeviceInfo.keyboard.dwKeyboardMode << endl;
                cout << "Number of function keys: " << rdiDeviceInfo.keyboard.dwNumberOfFunctionKeys << endl;
                cout << "Number of indicators: " << rdiDeviceInfo.keyboard.dwNumberOfIndicators << endl;
                cout << "Number of keys total: " << rdiDeviceInfo.keyboard.dwNumberOfKeysTotal << endl;
                cout << "Type of the keyboard: " << rdiDeviceInfo.keyboard.dwType << endl;
                cout << "Subtype of the keyboard: " << rdiDeviceInfo.keyboard.dwSubType << endl;
            }
            else
            {
                cout << endl << "Displaying device " << i + 1 << " information. (HID)" << endl;
                wcout << L"Device Name: " << wcDeviceName << endl;
                cout << "Vendor Id: " << rdiDeviceInfo.hid.dwVendorId << endl;
                cout << "Product Id: " << rdiDeviceInfo.hid.dwProductId << endl;
                cout << "Version No: " << rdiDeviceInfo.hid.dwVersionNumber << endl;
                cout << "Usage for the device: " << rdiDeviceInfo.hid.usUsage << endl;
                cout << "Usage Page for the device: " << rdiDeviceInfo.hid.usUsagePage << endl;
            }

            delete[] wcDeviceName;
        }

        delete[] pRawInputDeviceList;

        cout << endl << "Finished." << endl;

        printf("\n\n\n");
    }
}