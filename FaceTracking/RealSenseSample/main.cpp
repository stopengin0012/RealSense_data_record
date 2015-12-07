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

// ��ʕt�����֘A
bool showAngle = false;	// ��̊p�x���\���p
bool showEyeState = false;	// �ڂ̏�ԏ��\���p
bool showLandmarks = true;


// �^�撲���p
bool recordAdjustment = false; // true�Ȃ�Β�������
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
		//�w�b�_�[�ƂȂ��s��
		ofs_landmark.open("../csvfiles/" + newtime + "_landmark.csv");

		ofs_landmark
			<< "datetime,frame";

		
		//��ʈʒu�ł̊��Landmark���
		for (int s = 0; s < 78; s++) { ofs_landmark << "," << s << "._image_x," << s << "._image_y"; }
		

		
		//��ʈʒu�ł̊��Landmark���
		for (int s = 0; s < 78; s++) { ofs_landmark << "," << s << "._world_x,"  <<  s << "._world_y," << s << "._world_z"; }
		
		
		ofs_landmark << endl;

#pragma endregion LandmarkCSVFileInit

#pragma region ExpressionCSVFileInit

		//Expression
		//�w�b�_�[�ƂȂ��s��
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

		// SenseManager�𐶐�����
		senseManager = PXCSenseManager::CreateInstance();
		if (senseManager == 0) {
			throw std::runtime_error("SenseManager�̐����Ɏ��s���܂���");
		}

		//���ӁF�\��o���s���ꍇ�́A�J���[�X�g���[����L�������Ȃ�
		
		// �J���[�X�g���[����L���ɂ���
		pxcStatus sts = senseManager->EnableStream(PXCCapture::StreamType::STREAM_TYPE_COLOR, COLOR_WIDTH, COLOR_HEIGHT, COLOR_FPS);
		if (sts<PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("�J���[�X�g���[���̗L�����Ɏ��s���܂���");
		}
		

		initializeFace();



	}

	void initializeFace()
	{
		// �猟�o��L���ɂ���
		auto sts = senseManager->EnableFace();
		if (sts<PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("�猟�o�̗L�����Ɏ��s���܂���");
		}

		/*
		// �ǉ��F�\��o��L���ɂ���
		sts = senseManager->EnableEmotion();
		if (sts<PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("�\��o�̗L�����Ɏ��s���܂���");
		}
		*/

		//�猟�o��𐶐�����
		PXCFaceModule* faceModule = senseManager->QueryFace();
		if (faceModule == 0) {
			throw std::runtime_error("�猟�o��̍쐬�Ɏ��s���܂���");
		}

		//�猟�o�̃v���p�e�B���擾
		PXCFaceConfiguration* config = faceModule->CreateActiveConfiguration();
		if (config == 0) {
			throw std::runtime_error("�猟�o�̃v���p�e�B�擾�Ɏ��s���܂���");
		}

		config->SetTrackingMode(PXCFaceConfiguration::TrackingModeType::FACE_MODE_COLOR_PLUS_DEPTH);
		config->ApplyChanges();


		// �p�C�v���C��������������
		sts = senseManager->Init();
		if (sts<PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("�p�C�v���C���̏������Ɏ��s���܂���");
		}

		// �f�o�C�X���̎擾
		auto device = senseManager->QueryCaptureManager()->QueryDevice();
		if (device == 0) {
			throw std::runtime_error("�f�o�C�X�̎擾�Ɏ��s���܂���");
		}



		// �~���[�\���ɂ���
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
		config->pose.isEnabled = true;					//�ǉ��F��̎p�����擾���\�ɂ���
		config->pose.maxTrackedFaces = POSE_MAXFACES;	//�ǉ��F��l�܂ł̎p�����擾�\�ɐݒ肷��

		config->landmarks.isEnabled = true;		//�����h�}�[�N�̎擾
		config->landmarks.maxTrackedFaces = LANDMARK_MAXFACES;

		config->QueryPulse()->Enable();		//�ǉ��F�S���̌��o��L���ɂ���
		config->QueryPulse()->properties.maxTrackedFaces = PULSE_MAXFACES; //�ǉ��F�S�������o�ł���l����ݒ肷��

		config->QueryExpressions()->Enable();    //��̕\�o���̗L����
		config->QueryExpressions()->EnableAllExpressions();    //���ׂĂ̕\�o���̗L����
		config->QueryExpressions()->properties.maxTrackedFaces = 2;    //��̕\�o���̍ő�F���l��
		config->ApplyChanges();

		faceData = faceModule->CreateOutput();
	}

	void run()
	{
		// ���C�����[�v
		while (1) {
			// �t���[���f�[�^���X�V����
			updateFrame();

			// �\������
			auto ret = showImage();
			if (!ret) {
				break;
			}

			//�^�掞��FPS����
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
	* �����񒆂��當������������ĕʂ̕�����ɒu������
	* @param str  : �u���Ώۂ̕�����B�㏑����܂��B
	* @param from : ����������
	* @param to   : �u����̕�����
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
		// �t���[�����擾����
		pxcStatus sts = senseManager->AcquireFrame(false);
		if (sts < PXC_STATUS_NO_ERROR) {
			return;
		}

		updateFaceFrame();

		// �t���[�����������
		senseManager->ReleaseFrame();

		// �t���[�����[�g��\������
		showFps();
	}

	void updateFaceFrame() {
		// �t���[���f�[�^���擾����
		const PXCCapture::Sample *sample = senseManager->QuerySample();
		if (sample) {
			// �e�f�[�^��\������
			updateColorImage(sample->color);
		}


		//SenceManager���W���[���̊�̃f�[�^���X�V����
		faceData->Update();

		//���o������̐����擾����
		int numFaces = faceData->QueryNumberOfDetectedFaces();
		if (numFaces > 1) {
			numFaces = 1;
		}

		//��̗̈�������l�p�`��p�ӂ���
		PXCRectI32 faceRect = { 0 };

		//�ǉ��F��̎p�������i�[���邽�߂̕ϐ���p�ӂ���
		PXCFaceData::PoseEulerAngles poseAngle[POSE_MAXFACES] = { 0 }; //��l�ȏ�̎擾���\�ɂ���

		//��̊���̃f�[�^�A����ъp�x�̃f�[�^�̓��ꕨ��p��
		PXCFaceData::ExpressionsData *expressionData;
		PXCFaceData::ExpressionsData::FaceExpressionResult expressionResult;

		//��̃����h�}�[�N�i�����_�j�̃f�[�^�̓��ꕨ��p��
		PXCFaceData::LandmarksData *landmarkData[LANDMARK_MAXFACES];
		PXCFaceData::LandmarkPoint* landmarkPoints = nullptr;
		pxcI32 numPoints;



		//���ꂼ��̊炲�Ƃɏ��擾����ѕ`�揈�����s��
		for (int i = 0; i < numFaces; ++i) {

			//��̏����擾����
			auto face = faceData->QueryFaceByIndex(i);
			if (face == 0) {
				continue;
			}

#pragma region GetFacePoseAndHeartrate
			// ��̈ʒu���擾:Color�Ŏ擾����
			auto detection = face->QueryDetection();
			if (detection != 0) {
				//��̑傫�����擾����
				detection->QueryBoundingRect(&faceRect);
			}

			//��̈ʒu�Ƒ傫������A��̗̈�������l�p�`��`�悷��
			cv::rectangle(colorImage, cv::Rect(faceRect.x, faceRect.y, faceRect.w, faceRect.h), cv::Scalar(255, 0, 0));

			//�ǉ��F�S���Ɋւ����̏����擾����
			PXCFaceData::PulseData *pulse = face->QueryPulse();
			//�ǉ��F�S�������擾����
			pxcF32 hrate = pulse->QueryHeartRate();


			//�ǉ��F�|�[�Y(��̌������擾)�FDepth�g�p���̂�
			auto pose = face->QueryPose();
			if (pose != 0) {
				auto sts = pose->QueryPoseAngles(&poseAngle[i]);
				if (sts < PXC_STATUS_NO_ERROR) {
					throw std::runtime_error("QueryPoseAngles�Ɏ��s���܂���");
				}
			}

			if (showAngle) {
				//�ǉ��F��̎p�����(Yaw, Pitch, Roll)�̏��
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

			//��̃����h�}�[�N�ɂ��Ă̏���
			int numPointsByGroupType[7];
			int landmarkType[32];

			//�t�F�C�X�f�[�^���烉���h�}�[�N�i�����_�Q�j�ɂ��Ă̏��𓾂�
			landmarkData[i] = face->QueryLandmarks();
			if (landmarkData[i] != NULL)
			{
				//�����h�}�[�N�f�[�^���牽�̓����_���F���ł������𒲂ׂ�
				numPoints = landmarkData[i]->QueryNumPoints();
				//�F���ł��������_�̐������A�����_���i�[����C���X�^���X�𐶐�����
				landmarkPoints = new PXCFaceData::LandmarkPoint[numPoints];
				//�����h�}�[�N�f�[�^����A�����_�̈ʒu���擾�A�\��

				if (landmarkData[i]->QueryPoints(landmarkPoints)) {
					for (int j = 0; j < numPoints; j++) {
						{
							std::stringstream ss;
							ss << j;

							//��ʕ\������
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

			


			//��̃f�[�^���\�o���̃f�[�^�̏��𓾂�
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
					// �ڂ̓�����c�����ĕ\�������݂镔���i20151013�j
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

			//CSV�t�@�C���ւ̏����o��
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
				+ std::to_string(newt - old);	//�v���J�n������A�~���b�P�ʂŎ擾

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
			

			for (int s = 0; s < numPoints; s++) {	//Landmarks�̉�ʑΉ��̓񎟌��ʒu���
				ofs_landmark << "," << landmarkPoints[s].image.x << "," << landmarkPoints[s].image.y;
			}
			

			for (int s = 0; s < numPoints; s++) {	//Landmarks�̃f�o�C�X����̎O�����ʒu���
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

	// �J���[�摜���X�V����
	void updateColorImage(PXCImage* colorFrame)
	{
		if (colorFrame == 0) {
			return;
		}

		PXCImage::ImageInfo info = colorFrame->QueryInfo();

		// �f�[�^���擾����
		PXCImage::ImageData data;
		pxcStatus sts = colorFrame->AcquireAccess(PXCImage::Access::ACCESS_READ, PXCImage::PixelFormat::PIXEL_FORMAT_RGB24, &data);
		if (sts < PXC_STATUS_NO_ERROR) {
			throw std::runtime_error("�J���[�摜�̎擾�Ɏ��s");
		}

		// �f�[�^���R�s�[����
		colorImage = cv::Mat(info.height, info.width, CV_8UC3);
		memcpy(colorImage.data, data.planes[0], info.height * info.width * 3);



		// �f�[�^���������
		colorFrame->ReleaseAccess(&data);
	}

	// �摜��\������
	bool showImage()
	{
		// �\������
		cv::imshow(videoFrameTitle, colorImage);

		videorecord->VideoRecord(colorImage);

		// recordTimer���Ƃɘ^�������������
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

	// �t���[�����[�g�̕\��
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
	PXCEmotion* emotionDet = 0; //�ǉ��F�\��o�̌��ʂ��i�[���邽�߂̓��ꕨ

	const int COLOR_WIDTH = 640;
	const int COLOR_HEIGHT = 480;
	const int COLOR_FPS = 30;
	static const int NUM_TOTAL_EMOTIONS = 10;		//�ǉ��F�擾�ł���\���ъ���̂��ׂĂ̎�ނ̐�
	static const int NUM_PRIMARY_EMOTIONS = 7;		//�ǉ��F�\��̐�
	static const int NUM_SENTIMENT_EMOTIONS = 3;	//�ǉ��F����̐�

	static const int POSE_MAXFACES = 1;    //�ǉ��F��̎p�������擾�ł���ő�l����ݒ�
	const int PULSE_MAXFACES = 1;    //�ǉ��F�S�������o�ł���ő�l����ݒ�
	static const int LANDMARK_MAXFACES = 1;    //��̃����h�}�[�N�����擾�ł���ő�l��

	int recordTimer = 300000;	//�ꕪ��
	//int recordTimer = 5000;	//5�b

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
