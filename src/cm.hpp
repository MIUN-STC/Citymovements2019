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


void cm_update 
(
	cv::Point2f v [], //Tracker velocity
	int pe [], //Tracker persistence
	uint32_t n, //Number of trackers
	int persistence
)
{
	while (n--)
	{
		//If not tracking then decrease velocity
		//Persistence should be max if it's tracking.
		if (pe [n] < persistence)
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
	cv::Point2f p [], //Tracker position
	int pe [], //Tracker persistence
	uint32_t n, //Number of trackers
	cv::KeyPoint &kp, //Target position
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
	//Can be used to calculate tracker to target velocity.
	std::vector<cv::KeyPoint>& kp,
	//Tracker position
	//Can be used to check if the target is close to border.
	//Can be used to calculate tracker to target velocity.
	cv::Point2f p [],
	//Tracker velocity
	//Can be used to calculate which direction that the tracking is going.
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
		//Find a (target,tracker) pair
		int j = cm_find (p, pe, n, kp [i], proximity);
		if (j == -1) {continue;};
		//Calculate delta vector between target and tracker and use it as smoothed velocity
		v [j] = 0.9f * v [j] + (kp [i].pt - p [i]) * 0.1f;
		//Move tracker to target
		p [j] = kp [i].pt;
		//Set the tracker persistence high i.e. tracking is active.
		pe [j] = persistence;
		//Increase the trackers tracking duration.
		t [j] ++;
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
	int pe [], //Tracker persistence
	int t [], //Tracking duration
	uint32_t n, //Number of trackers
	struct cm_4way &way //Output people count
)
{
	bool departed_any = false;
	while (n--)
	{
		//Make sure that the target is realy gone.
		if (pe [n] != 1) {continue;}
		
		//Make sure that the target has not been tracking noise.
		if (t [n] < 30){continue;}
		
		//Reset target
		t [n] = 0;
		
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
