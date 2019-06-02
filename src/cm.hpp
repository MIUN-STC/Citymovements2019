#pragma once
#include "lepton.h"


//Departure zone size
size_t const CM_Size = 10;
//Departure edge zones
cv::Rect const CM_N (CM_Size, 0, LEP3_W - (CM_Size*2), CM_Size);
cv::Rect const CM_S (CM_Size, LEP3_H - CM_Size, LEP3_W - (CM_Size*2), CM_Size);
cv::Rect const CM_W (0, CM_Size, CM_Size, LEP3_H - (CM_Size*2));
cv::Rect const CM_E (LEP3_W - CM_Size, CM_Size, CM_Size, LEP3_H - (CM_Size*2));
//Departure corner zones
cv::Rect const CM_NW (0, 0, CM_Size, CM_Size);
cv::Rect const CM_NE (LEP3_W - CM_Size, 0, CM_Size, CM_Size);
cv::Rect const CM_SW (0, LEP3_H - CM_Size, CM_Size, CM_Size);
cv::Rect const CM_SE (LEP3_W - CM_Size, LEP3_H - CM_Size, CM_Size, CM_Size);


void cm_iv_adds (int r [], int v [], uint32_t n, int a)
{
	while (n--) 
	{
		r [n] = v [n] + a;
	}
}

void cm_Point2fv_adds (cv::Point2f r [], cv::Point2f v [], uint32_t n, float a)
{
	while (n--) 
	{
		r [n] = v [n] * a;
	}
}


int cm_pair 
(
	cv::Point2f p [], //Tracker position
	int t [], //Tracker tracking time
	int u [], //Tracker untracking time
	uint32_t n, //Number of trackers
	cv::KeyPoint &kp, //Target position
	float proximity,
	int persistence
)
{
	int imin = -1;
	float lmin = FLT_MAX;
	while (n--)
	{
		float l = (float)cv::norm (p [n] - kp.pt);
		//It is very important to update used trackers also.
		//If the tracker is being used then only track the target in proximity.
		if ((u [n] < persistence) && (l > proximity)) {continue;};
		if (l < lmin)
		{
			lmin = l;
			imin = (int)n;
		}
	}
	if (imin >= 0 && lmin > proximity)
	{
		u [imin] = 0;
		t [imin] = 0;
	}
	return imin;
}


//Producing continuous moving trackers.
//Tracking time can be used to filter out trackers that tracks noise.
//Tracker velocity can be used to calculate which direction that the tracking is going.
//Tracker position can be used to calculate tracker to target velocity.
//Tracker position can be used to check if the target is close to border.
//Input target keypoints is discontinuous target coordinate of interest that we want to track.
//Input target keypoints can be used to calculate tracker to target velocity.
//Proximity: How close the tracker can track a target.
//Persistence: How long time a non tracking tracker should look for a target in proximity.
void cm_track 
(
	std::vector<cv::KeyPoint>& kp, //Input target keypoints
	cv::Point2f p [], //Tracker position
	cv::Point2f v [], //Tracker velocity
	int t [], //Tracking time
	int u [], //Untracking time
	uint32_t n,//Number of tracker
	float proximity = 10.0f,
	int persistence = 100
)
{
	cm_Point2fv_adds (v, v, n, 0.95f);
	cm_iv_adds (u, u, n, 1);
	for (size_t i = 0; i < kp.size (); i++)
	{
		//Find a (target,tracker) pair
		int j = cm_pair (p, t, u, n, kp [i], proximity, persistence);
		if (j == -1) {continue;};
		//Calculate delta vector between target and tracker and use it as smoothed velocity
		v [j] = 0.9f * v [j] + (kp [i].pt - p [i]) * 0.1f;
		//Move tracker to target
		p [j] = kp [i].pt;
		//Increase the trackers tracking duration.
		t [j] ++;
		//Reset the untracking time
		u [j] = 0;
	}
}


//Counter for people going north, south, west, east
struct cm_4way
{
	uint32_t n;
	uint32_t s;
	uint32_t w;
	uint32_t e;
};


bool cm_countman 
(
	cv::Point2f p [], //Tracker position
	cv::Point2f v [], //Tracker velocity
	int u [], //Tracker persistence
	int t [], //Tracking duration
	uint32_t n, //Number of trackers
	struct cm_4way &way, //Output people count,
	int persistence,
	int confidence
)
{
	bool departed_any = false;
	while (n--)
	{
		//Make sure that the target is realy gone.
		if (u [n] < persistence) {continue;}
		
		//Make sure that the target has not been tracking noise.
		if (t [n] < confidence){continue;}
		
		//Flag variable for if the target has beed counted or not.
		bool departed = false;
		//Angle of the targets direction in degrees.
		//TODO: Check if we can use dod product instead, 
		//http://programmedlessons.org/VectorLessons/vch07/vch07_7.html
		float a = atan2f (v [n].y, v [n].x);
		float a360 = (180.0f / (float)M_PI) * a;
		
		//Check of the tracker are inside the departure zones.
		if (0) {}
		else if (CM_N.contains (p [n])) {way.n ++; departed = true;}
		else if (CM_S.contains (p [n])) {way.s ++; departed = true;}
		else if (CM_W.contains (p [n])) {way.w ++; departed = true;}
		else if (CM_E.contains (p [n])) {way.e ++; departed = true;}
		else if (CM_NE.contains (p [n])) 
		{
			if (a360 < -45.0f) {way.s ++;}
			else {way.e ++;}
			departed = true;
		}
		else if (CM_SE.contains (p [n])) 
		{
			if (a360 < 45.0f) {way.e ++;}
			else {way.n ++;}
			departed = true;
		}
		else if (CM_NW.contains (p [n])) 
		{
			if (a360 < 255.0f) {way.w ++;}
			else {way.n ++;}
			departed = true;
		}
		else if (CM_SW.contains (p [n])) 
		{
			if (a360 < 135.0f) {way.s ++;}
			else {way.w ++;}
			departed = true;
		}
		
		if (departed)
		{
			//Trackers with negative one tracking time duration
			//is trackers that thinks that the target left the premise.
			t [n] = -1;
			departed_any = true;
		}
	}
	return departed_any;
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
