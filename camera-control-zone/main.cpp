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

using namespace cv;
using namespace std;

atomic<bool> hasFragment = false;
atomic<bool> brak = false;
atomic<bool> stop = false;
atomic<bool> switchedOff = false;

atomic<int> switchOffRectTop = 0;
atomic<int> switchOffRectLeft = 0;
atomic<int> switchOffRectWidth = 0;
atomic<int> switchOffRectHeight = 0;

atomic<int> fragmentX = 0;
atomic<int> fragmentY = 0;
atomic<int> fragmentWidth = 0;
atomic<int> fragmentHeight = 0;

atomic<int> rectangleX = 0;
atomic<int> rectangleY = 0;
atomic<int> rectangleWidth = 0;
atomic<int> rectangleHeight = 0;

atomic<int> drawFragmentX1 = 0;
atomic<int> drawFragmentY1 = 0;
atomic<int> drawFragmentX2 = 0;
atomic<int> drawFragmentY2 = 0;

atomic<int> imageX = 0;
atomic<int> imageY = 0;
atomic<int> imageWidth = 0;
atomic<int> imageHeight = 0;
atomic<int> resizedImageX = 0;
atomic<int> resizedImageY = 0;
atomic<int> resizedImageWidth = 0;
atomic<int> resizedImageHeight = 0;
atomic<int> windowWidth = 0;
atomic<int> windowHeight = 0;

int main(int argc, char* argv[])
{
	cout << "camera control zone" << endl;
}