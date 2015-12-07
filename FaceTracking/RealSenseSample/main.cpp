#include <sstream>

#include <Windows.h>
#pragma comment(lib, "winmm.lib")

#include "pxcsensemanager.h"
#include "PXCFaceConfiguration.h"

#include <opencv2\opencv.hpp>

//Define to write CSV File
#include<fstream>
#include<iostream>
using namespace std;
std::ofstream ofs_landmark;
std::ofstream ofs_expression;
#include<ctime>


//
//VideoWriter
#include "OpenCVvRecoder.h"
OpenCVvRecoder *videorecord;
std::string videoname = "";
std::string videoFrameTitle = "Record";

// 画面付加情報関連
bool showAngle = false;	// 顔の角度情報表示用
bool showEyeState = false;	// 目の状態情報表示用
bool showLandmarks = true;


// 録画調整用
bool recordAdjustment = false; // trueならば調整する
const int fps = 30;
const double oneCut = 1000.0 / fps;
int sleepRemain = 0;
int countTime = 0;
int startRecordTime = 0;
int recordNum = 0;

class RealSenseAsenseManager
{
public:

	~RealSenseAsenseManager()
	{
		if (senseManager != 0) {
			senseManager->Release();
		}

		videorecord->ReleaseVideoWriter(videoFrameTitle);
		delete videorecord;
	}

	void initilize()
	{



		long_time = time(NULL);
		localtime_s(&localt, &long_time);
		asctime_s(timebuf, sizeof(timebuf), &localt);

		std::string newtime;

		newtime = std::to_string(localt.tm_year + 1900) + "_"
			+ std::to_string(localt.tm_mon + 1) + "_"
			+ std::to_string(localt.tm_mday) + "_"
			+ std::to_string(localt.tm_hour) + "_"
			+ std::to_string(localt.tm_min) + "_"
			+ std::to_string(localt.tm_sec);

#pragma region VideoFileInit
		videorecord = new OpenCVvRecoder;
		recordCount++;
		startTime = newtime;
		videoname = "../moviefiles/" + startTime  + "_" + std::to_string(recordCount) + ".avi";
		cvNamedWindow("Record", CV_WINDOW_NORMAL);
		videorecord->SetUpVideoWriter(videoFrameTitle, videoname);

#pragma endregion VideoFileInit

#pragma region LandmarkCSVFileInit
		//Landmark first column
		//ヘッダーとなる一行目
		ofs_landmark.open("../csvfiles/" + newtime + "_landmark.csv");

		ofs_landmark
			<< "datetime,frame";

		
		//画面位置での顔のLandmark情報
		for (int s = 0; s < 78; s++) { ofs_landmark << "," << s << "._image_x," << s << "._image_y"; }
		

		
		//画面位置での顔のLandmark情報
		for (int s = 0; s < 78; s++) { ofs_landmark << "," << s << "._world_x,"  <<  s << "._world_y," << s << "._world_z"; }
		
		
		ofs_landmark << endl;

#pragma endregion LandmarkCSVFileInit

#pragma region ExpressionCSVFileInit

		//Expression
		//ヘッダーとなる一行目
		ofs_expression.open("../csvfiles/" + newtime + "_expression.csv");

		ofs_expression
			<< "datetime,frame,"
			<< "yaw,"
			<< "pitch,"
			<< "roll,"
			<< "hrate,"
			<< "Rect.x,Rect.y,Rect.w,Rect.h,"
			<< "BROW_RAISER_LEFT, BROW_RAISER_RIGHT, BROW_LOWERER_LEFT, BROW_LOWERER_RIGHT,"
			<< "SMILE, KISS, MOUTH_OPEN, TONGUE_OUT,"
			<< "HEAD_TURN_LEFT, HEAD_TURN_RIGHT, HEAD_UP, HEAD_DOWN, HEAD_TILT_LEFT,"
			<< "EYES_CLOSED_LEFT, EYES_CLOSED_RIGHT, EYES_TURN_LEFT, EYES_TURN_RIGHT, EYES_UP, EYES_DOWN,"
			<< "PUFF_LEFT,PUFF_RIGHT,"
			<< endl;

#pragma endregion ExpressionCSVFileInit

		// SenseManagerを生成する
		senseManager = PXCSenseManager::CreateInstance();
		if (senseManager == 0) {
			throw std::runtime_error("SenseManagerの生成に失敗しました");
		}

		//注意：表情検出を行う場合は、カラーストリームを有効化しない
		
		// カラーストリームを有効にする
		pxcStatus sts = senseManager->EnableStream(PXCCapture::StreamType::STREAM_TYPE_COLOR, COLOR_WIDTH, COLOR_HEIGHT, COLOR_FPS);
		if (sts<PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("カラーストリームの有効化に失敗しました");
		}
		

		initializeFace();



	}

	void initializeFace()
	{
		// 顔検出を有効にする
		auto sts = senseManager->EnableFace();
		if (sts<PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("顔検出の有効化に失敗しました");
		}

		/*
		// 追加：表情検出を有効にする
		sts = senseManager->EnableEmotion();
		if (sts<PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("表情検出の有効化に失敗しました");
		}
		*/

		//顔検出器を生成する
		PXCFaceModule* faceModule = senseManager->QueryFace();
		if (faceModule == 0) {
			throw std::runtime_error("顔検出器の作成に失敗しました");
		}

		//顔検出のプロパティを取得
		PXCFaceConfiguration* config = faceModule->CreateActiveConfiguration();
		if (config == 0) {
			throw std::runtime_error("顔検出のプロパティ取得に失敗しました");
		}

		config->SetTrackingMode(PXCFaceConfiguration::TrackingModeType::FACE_MODE_COLOR_PLUS_DEPTH);
		config->ApplyChanges();


		// パイプラインを初期化する
		sts = senseManager->Init();
		if (sts<PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("パイプラインの初期化に失敗しました");
		}

		// デバイス情報の取得
		auto device = senseManager->QueryCaptureManager()->QueryDevice();
		if (device == 0) {
			throw std::runtime_error("デバイスの取得に失敗しました");
		}



		// ミラー表示にする
		device->SetMirrorMode(PXCCapture::Device::MirrorMode::MIRROR_MODE_HORIZONTAL);

		PXCCapture::DeviceInfo deviceInfo;
		device->QueryDeviceInfo(&deviceInfo);
		if (deviceInfo.model == PXCCapture::DEVICE_MODEL_IVCAM) {
			device->SetDepthConfidenceThreshold(1);
			device->SetIVCAMFilterOption(6);
			device->SetIVCAMMotionRangeTradeOff(21);
		}

		config->detection.isEnabled = true;
		config->detection.maxTrackedFaces = 1;
		config->pose.isEnabled = true;					//追加：顔の姿勢情報取得を可能にする
		config->pose.maxTrackedFaces = POSE_MAXFACES;	//追加：二人までの姿勢を取得可能に設定する

		config->landmarks.isEnabled = true;		//ランドマークの取得
		config->landmarks.maxTrackedFaces = LANDMARK_MAXFACES;

		config->QueryPulse()->Enable();		//追加：心拍の検出を有効にする
		config->QueryPulse()->properties.maxTrackedFaces = PULSE_MAXFACES; //追加：心拍を検出できる人数を設定する

		config->QueryExpressions()->Enable();    //顔の表出情報の有効化
		config->QueryExpressions()->EnableAllExpressions();    //すべての表出情報の有効化
		config->QueryExpressions()->properties.maxTrackedFaces = 2;    //顔の表出情報の最大認識人数
		config->ApplyChanges();

		faceData = faceModule->CreateOutput();
	}

	void run()
	{
		// メインループ
		while (1) {
			// フレームデータを更新する
			updateFrame();

			// 表示する
			auto ret = showImage();
			if (!ret) {
				break;
			}

			//録画時のFPS調整
			if (recordAdjustment) {
				
				countTime = (frame - startRecordTime);
				recordNum++;
				sleepRemain = (int)(recordNum * oneCut) - countTime;
				if (sleepRemain < 0) {
					sleepRemain = 0;
				}
				printf("%d, %d\n",recordNum, sleepRemain);
				Sleep(sleepRemain);

			}
			
		}
	}

private:

	void WriteInitialize() {
		char CDP[_MAX_PATH];
		GetCurrentDirectoryA(_MAX_PATH, CDP);
		currentdirPath = CDP;

		strReplace(currentdirPath, "\\", "/");
		std::cout << "currentdir: " << currentdirPath << std::endl;

	}

	/**
	* 文字列中から文字列を検索して別の文字列に置換する
	* @param str  : 置換対象の文字列。上書かれます。
	* @param from : 検索文字列
	* @param to   : 置換後の文字列
	*/
	void strReplace(std::string& str, const std::string& from, const std::string& to) {
		std::string::size_type pos = 0;
		while (pos = str.find(from, pos), pos != std::string::npos) {
			str.replace(pos, from.length(), to);
			pos += to.length();
		}
	}

	void updateFrame()
	{
		// フレームを取得する
		pxcStatus sts = senseManager->AcquireFrame(false);
		if (sts < PXC_STATUS_NO_ERROR) {
			return;
		}

		updateFaceFrame();

		// フレームを解放する
		senseManager->ReleaseFrame();

		// フレームレートを表示する
		showFps();
	}

	void updateFaceFrame() {
		// フレームデータを取得する
		const PXCCapture::Sample *sample = senseManager->QuerySample();
		if (sample) {
			// 各データを表示する
			updateColorImage(sample->color);
		}


		//SenceManagerモジュールの顔のデータを更新する
		faceData->Update();

		//検出した顔の数を取得する
		int numFaces = faceData->QueryNumberOfDetectedFaces();
		if (numFaces > 1) {
			numFaces = 1;
		}

		//顔の領域を示す四角形を用意する
		PXCRectI32 faceRect = { 0 };

		//追加：顔の姿勢情報を格納するための変数を用意する
		PXCFaceData::PoseEulerAngles poseAngle[POSE_MAXFACES] = { 0 }; //二人以上の取得を可能にする

		//顔の感情のデータ、および角度のデータの入れ物を用意
		PXCFaceData::ExpressionsData *expressionData;
		PXCFaceData::ExpressionsData::FaceExpressionResult expressionResult;

		//顔のランドマーク（特徴点）のデータの入れ物を用意
		PXCFaceData::LandmarksData *landmarkData[LANDMARK_MAXFACES];
		PXCFaceData::LandmarkPoint* landmarkPoints = nullptr;
		pxcI32 numPoints;



		//それぞれの顔ごとに情報取得および描画処理を行う
		for (int i = 0; i < numFaces; ++i) {

			//顔の情報を取得する
			auto face = faceData->QueryFaceByIndex(i);
			if (face == 0) {
				continue;
			}

#pragma region GetFacePoseAndHeartrate
			// 顔の位置を取得:Colorで取得する
			auto detection = face->QueryDetection();
			if (detection != 0) {
				//顔の大きさを取得する
				detection->QueryBoundingRect(&faceRect);
			}

			//顔の位置と大きさから、顔の領域を示す四角形を描画する
			cv::rectangle(colorImage, cv::Rect(faceRect.x, faceRect.y, faceRect.w, faceRect.h), cv::Scalar(255, 0, 0));

			//追加：心拍に関する顔の情報を取得する
			PXCFaceData::PulseData *pulse = face->QueryPulse();
			//追加：心拍数を取得する
			pxcF32 hrate = pulse->QueryHeartRate();


			//追加：ポーズ(顔の向きを取得)：Depth使用時のみ
			auto pose = face->QueryPose();
			if (pose != 0) {
				auto sts = pose->QueryPoseAngles(&poseAngle[i]);
				if (sts < PXC_STATUS_NO_ERROR) {
					throw std::runtime_error("QueryPoseAnglesに失敗しました");
				}
			}

			if (showAngle) {
				//追加：顔の姿勢情報(Yaw, Pitch, Roll)の情報
				{
					std::stringstream ss;
					ss << "Yaw:" << poseAngle[i].yaw;
					cv::putText(colorImage, ss.str(), cv::Point(faceRect.x, faceRect.y - 75), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2, CV_AA);
				}

				{
					std::stringstream ss;
					ss << "Pitch:" << poseAngle[i].pitch;
					cv::putText(colorImage, ss.str(), cv::Point(faceRect.x, faceRect.y - 50), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2, CV_AA);
				}

				{
					std::stringstream ss;
					ss << "Roll:" << poseAngle[i].roll;
					cv::putText(colorImage, ss.str(), cv::Point(faceRect.x, faceRect.y - 25), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2, CV_AA);
				}

				{
					std::stringstream ss;
					ss << "HeartRate:" << hrate;
					cv::putText(colorImage, ss.str(), cv::Point(faceRect.x, faceRect.y), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2, CV_AA);
				}
			}
#pragma endregion GetFacePoseAndHeartrate

#pragma region GetFaceLandmarks

			//顔のランドマークについての処理
			int numPointsByGroupType[7];
			int landmarkType[32];

			//フェイスデータからランドマーク（特徴点群）についての情報を得る
			landmarkData[i] = face->QueryLandmarks();
			if (landmarkData[i] != NULL)
			{
				//ランドマークデータから何個の特徴点が認識できたかを調べる
				numPoints = landmarkData[i]->QueryNumPoints();
				//認識できた特徴点の数だけ、特徴点を格納するインスタンスを生成する
				landmarkPoints = new PXCFaceData::LandmarkPoint[numPoints];
				//ランドマークデータから、特徴点の位置を取得、表示

				if (landmarkData[i]->QueryPoints(landmarkPoints)) {
					for (int j = 0; j < numPoints; j++) {
						{
							std::stringstream ss;
							ss << j;

							//画面表示処理
							if (/*landmarkPoints[j].source.alias != 0 &&*/ showLandmarks){
								cv::putText(colorImage, ss.str(),
									cv::Point(landmarkPoints[j].image.x, landmarkPoints[j].image.y),
									cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1, CV_AA);
							}
						}
					}
				}
			}

#pragma endregion GetFaceLandmarks

#pragma region GetFaceExpression

			PXCFaceData::ExpressionsData::FaceExpression expressionLabel[21] = {
				PXCFaceData::ExpressionsData::EXPRESSION_BROW_RAISER_LEFT,
				PXCFaceData::ExpressionsData::EXPRESSION_BROW_RAISER_RIGHT,
				PXCFaceData::ExpressionsData::EXPRESSION_BROW_LOWERER_LEFT,
				PXCFaceData::ExpressionsData::EXPRESSION_BROW_LOWERER_RIGHT,
				PXCFaceData::ExpressionsData::EXPRESSION_SMILE,
				PXCFaceData::ExpressionsData::EXPRESSION_KISS,
				PXCFaceData::ExpressionsData::EXPRESSION_MOUTH_OPEN,
				PXCFaceData::ExpressionsData::EXPRESSION_TONGUE_OUT,
				PXCFaceData::ExpressionsData::EXPRESSION_HEAD_TURN_LEFT,
				PXCFaceData::ExpressionsData::EXPRESSION_HEAD_TURN_RIGHT,
				PXCFaceData::ExpressionsData::EXPRESSION_HEAD_UP,
				PXCFaceData::ExpressionsData::EXPRESSION_HEAD_DOWN,
				PXCFaceData::ExpressionsData::EXPRESSION_HEAD_TILT_LEFT,
				PXCFaceData::ExpressionsData::EXPRESSION_EYES_CLOSED_LEFT,
				PXCFaceData::ExpressionsData::EXPRESSION_EYES_CLOSED_RIGHT,
				PXCFaceData::ExpressionsData::EXPRESSION_EYES_TURN_LEFT,
				PXCFaceData::ExpressionsData::EXPRESSION_EYES_TURN_RIGHT,
				PXCFaceData::ExpressionsData::EXPRESSION_EYES_UP,
				PXCFaceData::ExpressionsData::EXPRESSION_EYES_DOWN,
				PXCFaceData::ExpressionsData::EXPRESSION_PUFF_LEFT,
				PXCFaceData::ExpressionsData::EXPRESSION_PUFF_RIGHT
			};

			


			//顔のデータか表出情報のデータの情報を得る
			expressionData = face->QueryExpressions();
			if (expressionData != NULL)
			{
				for (int jj = 0; jj < 21; jj++) {
					expressionResult2[jj] = 0;
					if (expressionData->QueryExpression(expressionLabel[jj], &expressionResult)) {
						{
							expressionResult2[jj] = expressionResult.intensity;
							
						}
					}
				}


				if (showEyeState) {
					// 目の動きを把握して表示を試みる部分（20151013）
					int eye_turn_left = -1;
					int eye_turn_right = -1;
					int eye_up = -1;
					int eye_down = -1;
					int eye_closed_left = -1;
					int eye_closed_right = -1;

					if (expressionData->QueryExpression(PXCFaceData::ExpressionsData::EXPRESSION_EYES_TURN_LEFT, &expressionResult)) {
						eye_turn_left = expressionResult.intensity;
					}
					if (expressionData->QueryExpression(PXCFaceData::ExpressionsData::EXPRESSION_EYES_TURN_RIGHT, &expressionResult)) {
						eye_turn_right = expressionResult.intensity;
					}
					if (expressionData->QueryExpression(PXCFaceData::ExpressionsData::EXPRESSION_EYES_CLOSED_LEFT, &expressionResult)) {
						eye_closed_left = expressionResult.intensity;
					}
					if (expressionData->QueryExpression(PXCFaceData::ExpressionsData::EXPRESSION_EYES_CLOSED_RIGHT, &expressionResult)) {
						eye_closed_right = expressionResult.intensity;
					}
					if (expressionData->QueryExpression(PXCFaceData::ExpressionsData::EXPRESSION_EYES_UP, &expressionResult)) {
						eye_up = expressionResult.intensity;
					}
					if (expressionData->QueryExpression(PXCFaceData::ExpressionsData::EXPRESSION_EYES_DOWN, &expressionResult)) {
						eye_down = expressionResult.intensity;
					}

					int eyeX;
					int eyeY;

					if (eye_turn_left == eye_turn_right == -1) {
						eyeX = -9999;
					}
					else if (eye_turn_left == eye_turn_right) {
						eyeX = eye_turn_left;
					}
					else if (eye_turn_left < eye_turn_right) {
						eyeX = eye_turn_right;
					}
					else {
						eyeX = -eye_turn_left;
					}

					if (eye_up == eye_down == -1) {
						eyeY = -9999;
					}
					else if (eye_up == eye_down) {
						eyeY = eye_up;
					}
					else if (eye_up > eye_down) {
						eyeY = eye_up;
					}
					else {
						eyeY = -eye_down;
					}


					std::stringstream ss1;
					ss1 << "EYE_LOCATION: " << eyeX << ", " << eyeY;
					cv::putText(colorImage, ss1.str(), cv::Point(faceRect.x, faceRect.y + faceRect.h + 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2, CV_AA);

					std::stringstream ss2;
					ss2 << "EYE_CLOSED: " << eye_closed_left << ", " << eye_closed_right;
					cv::putText(colorImage, ss2.str(), cv::Point(faceRect.x, faceRect.y + faceRect.h + 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2, CV_AA);

				}


			}
#pragma endregion GetFaceExpression

#pragma region WriteCSVFiles

			//CSVファイルへの書き出し
			long_time = time(NULL);
			localtime_s(&localt, &long_time);
			asctime_s(timebuf, sizeof(timebuf), &localt);

			std::string newtime;

			static DWORD old = ::timeGetTime();

			auto newt = ::timeGetTime();


			newtime = std::to_string(localt.tm_year + 1900) + "_"
				+ std::to_string(localt.tm_mon) + "_"
				+ std::to_string(localt.tm_mday) + "_"
				+ std::to_string(localt.tm_hour) + ":"
				+ std::to_string(localt.tm_min) + ":"
				+ std::to_string(localt.tm_sec) + ":"
				+ std::to_string(newt - old);	//計測開始時から、ミリ秒単位で取得

			SYSTEMTIME st;
			GetLocalTime(&st);
			std::string newtime2;

			newtime2 = std::to_string(st.wYear) + "_"
				+ std::to_string(st.wMonth) + "_"
				+ std::to_string(st.wDay) + "_"
				+ std::to_string(st.wHour) + ":"
				+ std::to_string(st.wMinute) + ":"
				+ std::to_string(st.wSecond) + ":"
				+ std::to_string(st.wMilliseconds);



			{
				std::stringstream ss;
				ss << "Frame:" << std::to_string(newt - old);
				cv::putText(colorImage, ss.str(), cv::Point(50, 100), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2, CV_AA);
			}


			//Writing to CSV files about Landmark info
			frame = newt - old;

			ofs_expression
				<< newtime2 << ","					//datetime
				<< std::to_string(frame) << ","		//frame
				<< poseAngle[i].yaw << ","			//Angle
				<< poseAngle[i].pitch << ","
				<< poseAngle[i].roll << ","
				<< hrate << ","						//HeartRate
				<< faceRect.x << "," << faceRect.y << "," << faceRect.w << "," << faceRect.h << ",";


			for (int jj = 0; jj < 21; jj++) {
				ofs_expression << expressionResult2[jj];
				if (jj < 20) {
					ofs_expression << ",";
				}
			}

			ofs_expression << endl;

			//Writing to CSV files about Landmark info

			if (landmarkPoints == nullptr) {
				continue;
			}

			ofs_landmark
				<< newtime2 << ","							//datetime
				<< std::to_string(frame);					//frame
			

			for (int s = 0; s < numPoints; s++) {	//Landmarksの画面対応の二次元位置情報
				ofs_landmark << "," << landmarkPoints[s].image.x << "," << landmarkPoints[s].image.y;
			}
			

			for (int s = 0; s < numPoints; s++) {	//Landmarksのデバイスからの三次元位置情報
				pxcF32 land_x, land_y, land_z;

				if (fabsf(landmarkPoints[s].world.x) > 1)land_x = FloatRound(landmarkPoints[s].world.x, 4)  * (-1) / 1000;
				else land_x = FloatRound(landmarkPoints[s].world.x, 4);

				if (fabsf(landmarkPoints[s].world.y) > 1)land_y = FloatRound(landmarkPoints[s].world.y, 4) / 1000;
				else land_y = FloatRound(landmarkPoints[s].world.y, 4);

				if (fabsf(landmarkPoints[s].world.z) > 2)land_z = FloatRound(landmarkPoints[s].world.z, 4) / 1000;
				else land_z = FloatRound(landmarkPoints[s].world.z, 4);

	
				ofs_landmark << "," << FloatRound(land_x, 4) << "," << FloatRound(land_y, 4) << "," << FloatRound(land_z, 4);
			}

			ofs_landmark << endl;

#pragma endregion WriteCSVFiles

		}


	}

	
	pxcF32 FloatRound(pxcF32 f, int roundNum) {

		pxcF32 ret = 0;
		int _r;
		if (f == 0) {
			return 0;
		}
		else {
			_r = (int)(f * (pow(10, roundNum)));

			ret = _r / pow(10, roundNum);

			return ret;
		}

	}

	// カラー画像を更新する
	void updateColorImage(PXCImage* colorFrame)
	{
		if (colorFrame == 0) {
			return;
		}

		PXCImage::ImageInfo info = colorFrame->QueryInfo();

		// データを取得する
		PXCImage::ImageData data;
		pxcStatus sts = colorFrame->AcquireAccess(PXCImage::Access::ACCESS_READ, PXCImage::PixelFormat::PIXEL_FORMAT_RGB24, &data);
		if (sts < PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("カラー画像の取得に失敗");
		}

		// データをコピーする
		colorImage = cv::Mat(info.height, info.width, CV_8UC3);
		memcpy(colorImage.data, data.planes[0], info.height * info.width * 3);



		// データを解放する
		colorFrame->ReleaseAccess(&data);
	}

	// 画像を表示する
	bool showImage()
	{
		// 表示する
		cv::imshow(videoFrameTitle, colorImage);

		videorecord->VideoRecord(colorImage);

		// recordTimerごとに録画を完了させる
		if (frame - recordTimer*recordCount > 0) {
			videorecord->ReleaseVideoWriter(videoFrameTitle);
			// delete videorecord;

			// videorecord = new OpenCVvRecoder;
			recordCount++;
			videoname = "../moviefiles/" + startTime + "_" + std::to_string(recordCount) + ".avi";
			videorecord->SetUpVideoWriter(videoFrameTitle, videoname);
			startRecordTime = frame;
			countTime = 0;
			recordNum = 0;
		}
		

		int c = cv::waitKey(10);
		if ((c == 27) || (c == 'q') || (c == 'Q')) {
			// ESC|q|Q for Exit
			return false;
		}

		return true;
	}

	// フレームレートの表示
	void showFps()
	{
		static DWORD oldTime = ::timeGetTime();
		static int fps = 0;
		static int count = 0;

		count++;

		auto _new = ::timeGetTime();
		if ((_new - oldTime) >= 1000) {
			fps = count;
			count = 0;

			oldTime = _new;
		}

		std::stringstream ss;
		ss << "fps:" << fps;
		cv::putText(colorImage, ss.str(), cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 0, 255), 2, CV_AA);
	}

private:

	//cv::Mat colorImage;
	cv::Mat colorImage;
	PXCSenseManager* senseManager = 0;
	PXCFaceData* faceData = 0;
	PXCEmotion* emotionDet = 0; //追加：表情検出の結果を格納するための入れ物

	const int COLOR_WIDTH = 640;
	const int COLOR_HEIGHT = 480;
	const int COLOR_FPS = 30;
	static const int NUM_TOTAL_EMOTIONS = 10;		//追加：取得できる表情および感情のすべての種類の数
	static const int NUM_PRIMARY_EMOTIONS = 7;		//追加：表情の数
	static const int NUM_SENTIMENT_EMOTIONS = 3;	//追加：感情の数

	static const int POSE_MAXFACES = 1;    //追加：顔の姿勢情報を取得できる最大人数を設定
	const int PULSE_MAXFACES = 1;    //追加：心拍を検出できる最大人数を設定
	static const int LANDMARK_MAXFACES = 1;    //顔のランドマーク情報を取得できる最大人数

	int recordTimer = 300000;	//一分間
	//int recordTimer = 5000;	//5秒

	int recordCount = 0;
	std::string startTime;
	long frame;

	std::string header = "";

	struct tm localt;
	time_t long_time;
	char timebuf[32];
	errno_t err;
	int expressionResult2[21];

	string currentdirPath;
	//const int COLOR_WIDTH = 1920;
	//const int COLOR_HEIGHT = 1080;
	//const int COLOR_FPS = 30;
};

void main()
{
	try {
		RealSenseAsenseManager asenseManager;
		asenseManager.initilize();
		asenseManager.run();
	}
	catch (std::exception& ex) {
		videorecord->ReleaseVideoWriter(videoFrameTitle);
		delete videorecord;
		std::cout << ex.what() << std::endl;
	}
}
