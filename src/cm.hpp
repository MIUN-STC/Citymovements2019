#pragma once
#include "lepton.h"


//Size of the boxes for counting people.
size_t const CM_Size = 10;
//Setup edge boxes dimension and coordinate for counting people.
cv::Rect const CM_N (CM_Size, 0, LEP3_W - (CM_Size*2), CM_Size);
cv::Rect const CM_S (CM_Size, LEP3_H - CM_Size, LEP3_W - (CM_Size*2), CM_Size);
cv::Rect const CM_W (0, CM_Size, CM_Size, LEP3_H - (CM_Size*2));
cv::Rect const CM_E (LEP3_W - CM_Size, CM_Size, CM_Size, LEP3_H - (CM_Size*2));
//Setup corner boxes dimension and coordinate for counting people.
cv::Rect const CM_NW (0, 0, CM_Size, CM_Size);
cv::Rect const CM_NE (LEP3_W - CM_Size, 0, CM_Size, CM_Size);
cv::Rect const CM_SW (0, LEP3_H - CM_Size, CM_Size, CM_Size);
cv::Rect const CM_SE (LEP3_W - CM_Size, LEP3_H - CM_Size, CM_Size, CM_Size);


void cm_update 
(
	cv::Point2f v [], 
	int pe [], 
	uint32_t n, 
	int persistence_max
)
{
	while (n--)
	{
		//If not tracking then decrease velocity
		//Persistence should be max if it's tracking.
		if (pe [n] < persistence_max)
		{
			v [n] = v [n] * 0.95f;
		}
		//Decrease tracker persistence if it's positive.
		//Tracker that does not track will lose interest and 
		//release it self to track other targets when persistence reaches 0.
		if (pe [n] > 0) 
		{
			pe [n] -= 1;
		};
	}
}


int cm_find 
(
	cv::Point2f p [],
	int pe [], 
	uint32_t n, 
	cv::KeyPoint &kp, 
	float proximity
)
{
	int imin = -1;
	float lmin = FLT_MAX;
	while (n--)
	{
		float l = (float)cv::norm (p [n] - kp.pt);
		//It is very important to update used trackers also.
		//If the tracker is being used then only track the target in proximity.
		if ((pe [n] > 0) && (l > proximity)) {continue;};
		if (l < lmin)
		{
			lmin = l;
			imin = (int)n;
		}
	}
	return imin;
}


//Producing a continuous coordinate attached to persistent ID.
void cm_track 
(
	//Input target keypoints
	//Discontinuous target coordinate of interest that we want to track.
	std::vector<cv::KeyPoint>& kp,
	//Tracker position
	//Can be used to check if the target is close to border.
	//Can be used to calculate velocity.
	cv::Point2f p [],
	//Tracker velocity
	//Can be used to calculate the angle.
	cv::Point2f v [],
	//Tracker persistence
	// 0          : Tracker has no target and is not tracking.
	// 1 .. Max-1 : Tracker has a target but can not find it.
	// Max        : Tracker has a target and is tracking.
	int pe [],
	//Tracking time
	//Can be used to filter out trackers that tracks noise.
	int t [],
	//Number of tracker
	uint32_t n,
	//How close the tracker can track a target
	float proximity = 10.0f,
	//How long time a non tracking tracker should look for a target in proximity.
	int persistence = 100
)
{
	cm_update (v, pe, n, persistence);
	for (size_t i = 0; i < kp.size (); i++)
	{
		int j = cm_find (p, pe, n, kp [i], proximity);
		if (j == -1) {continue;};
		v [j] = 0.9f * v [j] + (kp [i].pt - p [i]) * 0.1f;
		p [j] = kp [i].pt;
		pe [j] = persistence;
		t [j] ++;
	}
}


//People count information for North, South, West, East.
struct cm_4way
{
	size_t n;
	size_t s;
	size_t w;
	size_t e;
};


bool cm_countman 
(
	cv::Point2f p [], 
	cv::Point2f v [],
	int pe [], 
	int t [], 
	uint32_t n, 
	struct cm_4way &way
)
{
	bool way_update = false;
	while (n--)
	{
		//Check if the target has been gone for a while.
		//This is making sure that the target is not deciding to go
		//back when it is outside the view of camera.
		//Check if the target has been tracked for a while. 
		if (pe [n] != 1) {continue;}
		if (t [n] < 30){continue;}
		t [n] = 0;
		//Flag variable for if the target has beed counted or not.
		bool border = false;
		//Angle of the targets direction in degrees.
		float a = atan2f (v [n].y, v [n].x);
		float a360 = (180.0f / (float)M_PI) * a;
		
		//Check if the target is whithin the counting box.
		//And angle of departure in the corner.
		if (0) {}
		else if (CM_N.contains (p [n])) {way.n ++; border = true;}
		else if (CM_S.contains (p [n])) {way.s ++; border = true;}
		else if (CM_W.contains (p [n])) {way.w ++; border = true;}
		else if (CM_E.contains (p [n])) {way.e ++; border = true;}
		else if (CM_NE.contains (p [n])) 
		{
			if (a360 < -45.0f) {way.s ++;}
			else {way.e ++;}
			border = true;
		}
		else if (CM_SE.contains (p [n])) 
		{
			if (a360 < 45.0f) {way.e ++;}
			else {way.n ++;}
			border = true;
		}
		else if (CM_NW.contains (p [n])) 
		{
			if (a360 < 255.0f) {way.w ++;}
			else {way.n ++;}
			border = true;
		}
		else if (CM_SW.contains (p [n])) 
		{
			if (a360 < 135.0f) {way.s ++;}
			else {way.w ++;}
			border = true;
		}
		
		if (border)
		{
			t [n] = -1;
			way_update = true;
		}
	}
	return way_update;
}


void cm_4way_print (struct cm_4way &way)
{
	printf ("N : %d\n", way.n);
	printf ("S : %d\n", way.s);
	printf ("W : %d\n", way.w);
	printf ("E : %d\n", way.e);
	printf ("\n");
	fflush (stdout);
}
