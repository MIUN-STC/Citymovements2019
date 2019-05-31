#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <vector>
#include <errno.h>
#include <opencv4/opencv2/core/core.hpp>
#include <opencv4/opencv2/core/utility.hpp>
#include <opencv4/opencv2/highgui/highgui.hpp>
#include <opencv4/opencv2/imgproc/imgproc.hpp>
#include <opencv4/opencv2/features2d/features2d.hpp>
#include <opencv4/opencv2/video/background_segm.hpp>

#include "debug.h"
#include "lepton.h"
#include "cm.hpp"

int main(int argc, char *argv[])
{
	while (1)
	{
		int c = getopt (argc, argv, "");
		if (c == - 1) {break;}
		switch (c)
		{
			default:
			break;
		}
	}
	
	cv::SimpleBlobDetector::Params Params;
	Params.minThreshold = 60;
	Params.maxThreshold = 255;
	Params.filterByColor = false;
	Params.blobColor = 100;
	Params.filterByArea = true;
	Params.minArea = 10;
	Params.maxArea = 100;
	Params.filterByCircularity = false;
	Params.minCircularity = 0.1f;
	Params.maxCircularity = 1.0;
	Params.filterByConvexity = false;
	Params.minConvexity = 0.0;
	Params.maxConvexity = 0.5;
	Params.filterByInertia = false;
	Params.minInertiaRatio = 0.0;
	Params.maxInertiaRatio = 0.5;
	cv::Ptr<cv::BackgroundSubtractor> Subtractor = cv::createBackgroundSubtractorMOG2 ();
	cv::Ptr<cv::SimpleBlobDetector> Blobber = cv::SimpleBlobDetector::create (Params);
	std::vector<cv::KeyPoint> Targets;
	
	struct
	{
		#define TRACKER_COUNT 4
		cv::Point2f p [TRACKER_COUNT]; //Position
		cv::Point2f v [TRACKER_COUNT]; //Delta
		int pe [TRACKER_COUNT]; //Persistance
		int t [TRACKER_COUNT]; //Time tracked
	} tracker;
	struct cm_4way way;
	memset (&way, 0, sizeof (way));
	memset (&tracker, 0, sizeof (tracker));
	
	cv::Mat m_source (LEP3_H, LEP3_W, CV_16U);
	cv::Mat m_fg (LEP3_H, LEP3_W, CV_8U);
	cv::Mat m_b (LEP3_H, LEP3_W, CV_8U);
	
	printf ("While loop start\n");
	while (1)
	{
		int r = read (STDIN_FILENO, m_source.ptr (), LEP3_WH*sizeof(uint16_t));
		if (r != LEP3_WH*sizeof(uint16_t)) {fprintf (stderr, "Reading the frame from stdin is not correct size\n");}
		Subtractor->apply (m_source, m_fg);
		cv::GaussianBlur (m_fg, m_b, cv::Size (11, 11), 3.5, 3.5);
		Blobber->detect (m_b, Targets);
		cm_track (Targets, tracker.p, tracker.v, tracker.pe, tracker.t, TRACKER_COUNT);
		bool counted = cm_countman (tracker.p, tracker.v, tracker.pe, tracker.t, TRACKER_COUNT, way);
		if (counted) {cm_4way_print (way);}
		for (size_t i = 0; i < TRACKER_COUNT; i++)
		{
			if (tracker.t [i] != -1) {continue;}
			printf ("Tracker %i departured\n", i);
			memset (&tracker, 0, sizeof (tracker));
			tracker.p [i] = {(float)LEP3_W / 2.0f, (float)LEP3_H / 2.0f};
		}
	}
	return 0;
}