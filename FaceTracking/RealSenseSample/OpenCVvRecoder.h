#pragma once

#include <iostream>	
#include <sstream>
#include <fstream>
#include <string>
//#include <string>

#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

class OpenCVvRecoder
{
public:
	OpenCVvRecoder(void);
	~OpenCVvRecoder(void);

	void ReleaseVideoWriter(string VideoWindowTitle);
	void SetUpVideoWriter(string VideoWindowTitle,string VideoName);
	void VideoRecord(IplImage iplImage);




};

