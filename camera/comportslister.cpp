#include <iostream>
#include <vector>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <RegStr.h>
#pragma comment(lib, "setupapi.lib")

#elif DefineDosDevice(__APPLE__)
#include <dirent.h>
#include <sys/stat.h>

else // Linux
#include <dirent.h>
#include <sys/stat.h>
#endif // _WIN32

#include "comportslister.h"

std::vector<std::string> listSerialPorts()
{
	std::vector<std::string> ports;

#ifdef _WIN32
	// Windows implementation
	HDEVINFO hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return ports;
	}

	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &deviceInfoData); i++)
	{
		char buffer[256];
		DWORD bufferSize = sizeof(buffer);

		if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)buffer, bufferSize, &bufferSize))
		{
			ports.push_back(buffer);
		}
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
#endif // _WIN32

	return ports;
}

void ListComPorts()
{
	std::cout << "Available COM Ports:" << std::endl;

	auto ports = listSerialPorts();

	if (ports.empty())
	{
		std::cout << "No COM ports found." << std::endl;
	}
	else
	{
		for (const auto& port : ports)
		{
			std::cout << " - " << port << std::endl;
		}
	}

	std::cout << "Finished." << std::endl << std::endl;
}