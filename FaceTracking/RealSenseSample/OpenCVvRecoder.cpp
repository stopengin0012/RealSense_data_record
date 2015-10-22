#include "OpenCVvRecoder.h"


//画像サイズ
int Width  = 640;
int Height = 480;

//aviファイル設定
double fps = 20;
CvVideoWriter* ForVideoWriter;
VideoWriter v;

OpenCVvRecoder::OpenCVvRecoder(void)
{
	//v = new VideoWriter;
}


OpenCVvRecoder::~OpenCVvRecoder(void)
{
	//delete v;
}

void OpenCVvRecoder::SetUpVideoWriter(string VideowindowTitle, string VideoName){
	//VideoWriter
	//cvReleaseVideoWriter (&VideoWriter);

	//movie
	std::string str = VideoName;
	
	int len = str.length();
	char* fname = new char[len+1];
	
	memcpy(fname, str.c_str(), len+1);
	
	/*
	if(v.isOpened()){
		v.release();
	}
	*/

	v.open(fname, CV_FOURCC('X', 'V', 'I', 'D'), fps, cvSize(Width, Height), 1);	//構造体の作成
	//v.open(fname, CV_FOURCC('D','I','B',' '), fps, cvSize(Width, Height), 1 );	//構造体の作成
	//v.open(fname, CV_FOURCC('W','M','V','3'), fps, cvSize(Width, Height), 1 );	//構造体の作成
	//v.open(fname, -1, fps, cvSize(Width, Height), 1 );	//構造体の作成(選択の場合)
}

void OpenCVvRecoder::ReleaseVideoWriter(string VideoWindowTitle){
	try{
		int len = VideoWindowTitle.length();
		char* fname = new char[len+1];
		memcpy(fname, VideoWindowTitle.c_str(), len+1);
		
		
		//cvReleaseVideoWriter(&ForVideoWriter);
		v.release();
		//v.~VideoWriter();
		//cvDestroyWindow (fname);
		//cvDestroyAllWindows();
		//VideoWriter = null;
		//delete v;

	}catch(std::exception& ex){
		std::cout<< "movie memory release fault:" << ex.what() << std::endl;
		fflush(stdin);//input buffer clear
		char c;
		while(std::cin.get(c)) if ((int)c==10) break;
	}
}

void OpenCVvRecoder::VideoRecord(IplImage iplImage){
	//１画面分の書込
	v.write(&iplImage);
}