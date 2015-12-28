#pragma once

#include <cv.h>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "highgui.h"

#include "histogrammer.h"
#include "globalSettings.h"

using namespace cv;
using namespace std;

namespace st {

//*************************************************************************************************
// ----- This class is used to perform B/F segmentation (the version with normalizing image)
//*************************************************************************************************
class BackGroundRemover {

	//________________________________________________________________________________________________
	private:
		int accumSize, bins, smoothSize;
		int channels[1], numOfBins[1];
		float hrangers[2];
		const float* ranges[1];
		vector<MatND> allHists;
		deque<MatND> accum;
		bool applyMorphologic;
		Mat normalizedFrame, morphElem, resultMask;

	//________________________________________________________________________________________________
	public:

		//=========================================================================================
		BackGroundRemover (int accumSize, int bins, int smoothSize, bool applyMorphologic = false) {
			this->applyMorphologic = applyMorphologic;
			this->accumSize = accumSize;
			this->bins = bins;
			this->smoothSize = smoothSize;
			numOfBins[0] = bins;
		
			channels[0] = 1;

			hrangers[0] = 0.0;
			hrangers[1] = 255.0;
			ranges[0] = hrangers;

			normalizedFrame = Mat::zeros(fSize, CV_8UC3);
			resultMask = Mat::zeros(fSize, CV_8UC1);

			if (applyMorphologic) {
				morphElem = Mat::ones(3, 3, CV_8U);
				morphElem.at<uchar>(0,0) = 0;
				morphElem.at<uchar>(2,0) = 0;
				morphElem.at<uchar>(0,2) = 0;
				morphElem.at<uchar>(2,2) = 0;
			}
		}

		//=========================================================================================
		Mat processFrame (Mat& frame) {
			normalizeImage(frame, normalizedFrame);
			if (smoothSize != -1) {
				smoothImage(normalizedFrame, smoothSize);
			}
			HistMethod(normalizedFrame, resultMask, true);
			if (applyMorphologic) {
				applyMorphological(resultMask, resultMask);
			}
			//imshow("M", resultMask);
			//foo (binMask, resized, frame);
			return resultMask;
		}

		//=========================================================================================
		void updateAccumulator (MatND& srcHist, MatND& accumulatedHist) {
			accum.push_back(srcHist.clone());
			int lh = int(accum.size());
			if (lh > accumSize) {
				accum.pop_front();
				lh--;
			}
			Mat sum = MatND::zeros(bins, 1, CV_32FC1);
			for (deque<MatND>::iterator itr = accum.begin(); itr != accum.end(); itr++) {
				sum += *itr;
			}
			sum /= lh;
			accumulatedHist = sum;
		}

		//=========================================================================================
		void HistMethod (Mat& frame, Mat& binMask, bool withAccumulator = true) {
			// calculate hist for green channel of normalized image
			MatND histG;
			calcHist(&frame, 1, channels, Mat(), histG, 1, numOfBins, ranges);
			MatND hMask = MatND::ones(bins, 1, CV_32FC1);
			if (withAccumulator) {
				allHists.push_back(histG.clone());
				if (isTheSameScene(histG)) {
					updateAccumulator(histG, histG);
				}
			}
			MatND smoothedHistG = Histogrammer::filterHist(histG);
			MatND histMask = Histogrammer::getHistMask(smoothedHistG);
			multiply(histG, histMask, histG);
			binMask = Histogrammer::backProj(frame, histG, 1, true);

			//imshow("m", binMask);
			//imshow("s", getSkeleton(binMask));
		}

		//=========================================================================================
		Mat getSkeleton (Mat& img) { // unused feature (time consuming)
			Mat skeleton(img.size(), CV_8UC1, Scalar(0));
			Mat temp, eroded;
			Mat element = getStructuringElement(MORPH_CROSS, Size(3, 3));
			bool done;		
			do {
				erode(img, eroded, element);
				dilate(eroded, temp, element);
				subtract(img, temp, temp);
				bitwise_or(skeleton, temp, skeleton);
				eroded.copyTo(img);
				done = (countNonZero(img) == 0);
			} while (!done);
			return skeleton;
		}

		//=========================================================================================
		inline void normalizeImage (Mat& input, Mat& output) {
			int b, g, r, s;
			int fH = input.rows;
			int fW = input.cols;
			if (input.isContinuous() && output.isContinuous()) {
				fW = fW * fH;
				fH = 1;
			}
			for (int i = 0; i < fH; i++) {
				uchar* inp = input.ptr<uchar>(i);
				uchar* outp = output.ptr<uchar>(i);
				for (int j = 0; j < fW; j++) {
					b = *inp++;
					g = *inp++;
					r = *inp++;
					s = r + g + b;
					if (s == 0) {
						s = 1;
					}
					*outp++ = 255 * b / s;
					*outp++ = 255 * g / s;
					*outp++ = 255 * r / s;
				}
			}
		}

		//=========================================================================================
		inline void smoothImage (Mat& input, int kernelSize) {
			double sigma = double(kernelSize)/3;
			GaussianBlur(input, input, Size(kernelSize,kernelSize), sigma, sigma);
		}

		//===============================================================================================
		bool isTheSameScene (MatND& hist) {
			return true;
		}

		//===============================================================================================
		inline void applyMorphological (Mat& input, Mat& output) {
			morphologyEx(input, output, MORPH_CLOSE, morphElem);
			output = input;
		}

		//=========================================================================================
		static void process(Mat& input, Mat& output) {
			output = input;
			for (int i = 0; i < 4; i++) {
				bitwise_not(output, output);
			}
		}

		//=========================================================================================
		static void oneLoop (Mat& input) {
			int b, g, r;
			int fH = input.rows;
			int fW = input.cols;
			if (input.isContinuous()) {
				fW = fW * fH;
				fH = 1;
			}
			for (int i = 0; i < fH; i++) {
				uchar* inp = input.ptr<uchar>(i);
				for (int j = 0; j < fW; j++) {
					b = *inp++;
					g = *inp++;
					r = *inp++;
				}
			}
		}

		//=========================================================================================
		~BackGroundRemover(void) {}


};



}