#pragma once

#include <opencv4/opencv2/imgproc/imgproc.hpp>
#include "lepton.h"
#include "cm.hpp"


#define STRACKER_COUNT 4
#define STRACKER_PERSISTENCE 100
#define STRACKER_BORDER_PERSISTENCE 40
#define STRACKER_BORDER_CONFIDENCE 40
#define STRACKER_PROXIMITY 40.0f


struct stracker
{
	cv::Point2f p [STRACKER_COUNT]; //Trackers position point (x, y)
	cv::Point2f v [STRACKER_COUNT]; //Trackers velocity vector (dx, dy)
	int t [STRACKER_COUNT]; //Trackers tracked duration for (t >= 0) and departed for (t = -1)
	int u [STRACKER_COUNT]; //Trackers untracked duration (u >= 0)
};

void sinit
(
	cv::Ptr<cv::BackgroundSubtractor> &subtractor,
	cv::Ptr<cv::SimpleBlobDetector> &blobber,
	struct stracker &tracker,
	struct cm_4way &way
)
{
	cv::SimpleBlobDetector::Params bparam;
	bparam.minThreshold = 60;
	bparam.maxThreshold = 255;
	bparam.filterByColor = false;
	bparam.blobColor = 100;
	bparam.filterByArea = true;
	bparam.minArea = 10;
	bparam.maxArea = 100;
	bparam.filterByCircularity = false;
	bparam.minCircularity = 0.1f;
	bparam.maxCircularity = 1.0;
	bparam.filterByConvexity = false;
	bparam.minConvexity = 0.0;
	bparam.maxConvexity = 0.5;
	bparam.filterByInertia = false;
	bparam.minInertiaRatio = 0.0;
	bparam.maxInertiaRatio = 0.5;
	subtractor = cv::createBackgroundSubtractorMOG2 ();
	blobber = cv::SimpleBlobDetector::create (bparam);
	memset (&way, 0, sizeof (way));
	memset (&tracker, 0, sizeof (tracker));
}


//Filter pipeline:
//0. FLIR Lepton 3 source : (physical world) -> (image)
//1. Background subtract  : (image) -> (image)
//2. Gaussian blur        : (image) -> (image)
//3. Blob detect          : (image) -> (position, blobsize)
//4. Track                : (position) -> (id, position, velocity, untracking duration, tracking duration)
//5. Departure count      : (id, position, velocity, untracking duration, tracking duration) -> (n, s, w, e, counted)
void sfilter 
(
	cv::Ptr<cv::BackgroundSubtractor> &subtractor,
	cv::Ptr<cv::SimpleBlobDetector> &blobber,
	cv::Mat &mat_source,
	cv::Mat &mat_fg,
	cv::Mat &mat_b,
	std::vector<cv::KeyPoint> &kp,
	struct stracker &tracker,
	struct cm_4way &way
)
{
	subtractor->apply (mat_source, mat_fg);
	cv::GaussianBlur (mat_fg, mat_b, cv::Size (11, 11), 3.5, 3.5);
	blobber->detect (mat_b, kp);
	
	//Reset departed trackers
	for (size_t i = 0; i < STRACKER_COUNT; i++)
	{
		if (tracker.t [i] != -1) {continue;}
		tracker.v [i] = {0.0f, 0.0f};
		tracker.p [i] = {(float)LEP3_W / 2.0f, (float)LEP3_H / 2.0f};
		tracker.t [i] = 0;
		tracker.u [i] = 0;
	}
	
	cm_track 
	(
		kp, 
		tracker.p, //Position point (x, y)
		tracker.v, //Velocity vector (dx, dy)
		tracker.t, //Tracking duration for (t >= 0) and departed for (t = -1)
		tracker.u, //Untracking duration for (y >= 0)
		STRACKER_COUNT, 
		STRACKER_PROXIMITY * STRACKER_PROXIMITY, 
		STRACKER_PERSISTENCE
	);
	
	cm_countman 
	(
		tracker.p, //Position point (x, y)
		tracker.v, //Velocity vector (dx, dy)
		tracker.t, //Tracking duration for (t >= 0), departed for (t = -1)
		tracker.u, //Untracking duration for (y >= 0)
		STRACKER_COUNT, 
		way, //North, East, West, South counter
		STRACKER_BORDER_PERSISTENCE, 
		STRACKER_BORDER_CONFIDENCE
	);
}


