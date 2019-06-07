#include <stdio.h>
#include <unistd.h> //getopt
#include <vector>

#include <opencv4/opencv2/core/core.hpp>
#include <opencv4/opencv2/core/utility.hpp>
#include <opencv4/opencv2/imgproc/imgproc.hpp>
#include <opencv4/opencv2/features2d/features2d.hpp>
#include <opencv4/opencv2/video/background_segm.hpp>

#include "debug.h"
#include "lepton.h"
#include "cm.hpp"
#include "shared.hpp"

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
	
	cv::Ptr<cv::BackgroundSubtractor> subtractor;
	cv::Ptr<cv::SimpleBlobDetector> blobber;
	cv::Mat mat_source (LEP3_H, LEP3_W, CV_16U);
	cv::Mat mat_fg (LEP3_H, LEP3_W, CV_8U);
	cv::Mat mat_b (LEP3_H, LEP3_W, CV_8U);
	std::vector<cv::KeyPoint> kp;
	struct stracker tracker;
	struct cm_4way way; //North, East, West, South counter
	
	sinit
	(
		subtractor, //background subtraction
		blobber, //blob detection
		tracker, //
		way //North, East, West, South counter
	);
	
	
	printf ("Citymovements tracking starts now\n");
	
	while (1)
	{
		int r = fread (mat_source.ptr (), LEP3_WH*sizeof(uint16_t), 1, stdin);
		if (r != 1) {fprintf (stderr, "Reading the frame from stdin is not correct size\n");}
		
		sfilter
		(
			subtractor, //background subtractor
			blobber, //blob detection
			mat_source, //camera source raw input
			mat_fg, //foreground of background subtraction
			mat_b, //blurred image
			kp, //target keypoints
			tracker, //
			way
		);
		
		if (way.counted > 0) {cm_4way_print (way);}
		
		for (size_t i = 0; i < STRACKER_COUNT; i++)
		{
			if (tracker.t [i] != -1) {continue;}
			printf ("Tracker %i departured\n", i);
		}
		
	}
	return 0;
}
