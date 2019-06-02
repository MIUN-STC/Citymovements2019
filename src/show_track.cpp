#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

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
#include "xgl.h"
#include "lepton.h"
#include "reader.h"
#include "cm.hpp"

#define APP_TEX_COUNT 1

#define APP_RENDER_C          1
#define APP_RENDER_TYPE       GL_UNSIGNED_BYTE
#define APP_RENDER_FORMAT     GL_RGB
#define APP_RENDER_INTFORMAT  GL_RGB
#define APP_RENDER_UNIT       0
#define APP_RENDER_MAG_FILTER GL_NEAREST
//#define APP_RENDER_MAG_FILTER GL_LINEAR

#define APP_WIN_X SDL_WINDOWPOS_UNDEFINED
#define APP_WIN_Y SDL_WINDOWPOS_UNDEFINED
#define APP_WIN_W LEP3_W
#define APP_WIN_H LEP3_H
#define APP_WIN_FLAGS SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
#define APP_WIN_NAME "Test background filter"
#define APP_SHADERF "src/rgb.glfs"
#define APP_SHADERV "src/rgb.glvs"


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
	
	{int r = SDL_Init (SDL_INIT_VIDEO); ASSERT_F (r == 0, "SDL_Init: %s", SDL_GetError());}
	
	SDL_Window * window;
	SDL_GLContext context;
	
	window = SDL_CreateWindow (APP_WIN_NAME, APP_WIN_X, APP_WIN_Y, APP_WIN_W, APP_WIN_H, APP_WIN_FLAGS);
	ASSERT_F (window, "SDL_CreateWindow: %s", SDL_GetError());
	
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 1);
	//SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute (SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
	context = SDL_GL_CreateContext (window);
	ASSERT_F (context, "SDL_GL_CreateContext: %s", SDL_GetError());
	SDL_GL_SetSwapInterval (1);// Use VSYNC

	printf ("%s\n", glGetString(GL_VERSION) );

	GLuint vbo;
	GLuint ebo;
	GLuint program;
	
	program = glCreateProgram ();XGL_ASSERT_ERROR;
	xgl_attach_shaderfile (program, APP_SHADERF, GL_FRAGMENT_SHADER);
	xgl_attach_shaderfile (program, APP_SHADERV, GL_VERTEX_SHADER);
	glLinkProgram (program);XGL_ASSERT_ERROR;
	glUseProgram (program);XGL_ASSERT_ERROR;
	xgl_program_print (program);
	
	float vertices [] = 
	{   //x      y     s      t
		-1.0f, -1.0f, 0.0f,  1.0f, // BL
		-1.0f,  1.0f, 0.0f,  0.0f, // TL
		 1.0f,  1.0f, 1.0f,  0.0f, // TR
		 1.0f, -1.0f, 1.0f,  1.0f  // BR
	};
	const GLint indicies [] = {0, 1, 2, 0, 2, 3};
	
	glGenBuffers (1, &(vbo));XGL_ASSERT_ERROR;
	glBindBuffer (GL_ARRAY_BUFFER, vbo);XGL_ASSERT_ERROR;
	glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);XGL_ASSERT_ERROR;
	glGenBuffers (1, &(ebo));XGL_ASSERT_ERROR;
	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, ebo);XGL_ASSERT_ERROR;
	glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indicies), indicies, GL_STATIC_DRAW);XGL_ASSERT_ERROR;
	{
		GLint pos_attr_loc = glGetAttribLocation (program, "in_Position");
		ASSERT (pos_attr_loc >= 0);
		printf ("pos_attr_loc %i \n", pos_attr_loc);
		glVertexAttribPointer ((GLuint)pos_attr_loc, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
		glEnableVertexAttribArray ((GLuint)pos_attr_loc);
		GLint tex_attr_loc = glGetAttribLocation (program, "in_Texcoord");
		ASSERT (tex_attr_loc >= 0);
		printf ("tex_attr_loc %i \n", tex_attr_loc);
		glVertexAttribPointer ((GLuint)tex_attr_loc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
		glEnableVertexAttribArray ((GLuint)tex_attr_loc);
	}
	glUseProgram (program);XGL_ASSERT_ERROR;
	glEnable (GL_BLEND);XGL_ASSERT_ERROR;
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);XGL_ASSERT_ERROR;
	

	cv::Mat m_source (LEP3_H, LEP3_W, CV_16U);
	cv::Mat m_fg (LEP3_H, LEP3_W, CV_8U);
	cv::Mat m_b (LEP3_H, LEP3_W, CV_8U);
	cv::Mat m_render (LEP3_H, LEP3_W, CV_8UC3);
	reader_start (m_source.ptr ());
	
	
	//Generate texture id for Lepton and Pallete.
	GLuint tex [APP_TEX_COUNT];
	glGenTextures (APP_TEX_COUNT, tex);XGL_ASSERT_ERROR;
	//Setup Lepton texture format.
	glActiveTexture (GL_TEXTURE0 + APP_RENDER_UNIT);XGL_ASSERT_ERROR;
	glBindTexture (GL_TEXTURE_2D, tex [APP_RENDER_UNIT]);XGL_ASSERT_ERROR;
	xgl_uniform1i_set (program, "tex", APP_RENDER_UNIT);
	//glPixelStorei(GL_UNPACK_ALIGNMENT, (M2.step & 3) ? 1 : 4);
	glTexImage2D (GL_TEXTURE_2D, 0, APP_RENDER_INTFORMAT, LEP3_W, LEP3_H, 0, APP_RENDER_FORMAT, APP_RENDER_TYPE, NULL);XGL_ASSERT_ERROR;
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);XGL_ASSERT_ERROR;
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);XGL_ASSERT_ERROR;
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, APP_RENDER_MAG_FILTER);XGL_ASSERT_ERROR;
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); XGL_ASSERT_ERROR;
	glGenerateMipmap (GL_TEXTURE_2D);XGL_ASSERT_ERROR;

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
		cv::Point2f p [TRACKER_COUNT]; //Trackers position
		cv::Point2f v [TRACKER_COUNT]; //Trackers delta
		int t [TRACKER_COUNT]; //Trackers tracked time
		int u [TRACKER_COUNT]; //Trackers untracked time
		float proximity;
		int persistence;
		int confidence;
	} tracker;
	struct cm_4way way;
	memset (&way, 0, sizeof (way));
	memset (&tracker, 0, sizeof (tracker));
	tracker.proximity = 10.0f;
	tracker.persistence = 100;
	tracker.confidence = 40;
	ASSERT (tracker.confidence < tracker.persistence);
	
	while (1)
	{
		SDL_Event event;
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				goto main_quit;
				
			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
				case SDL_WINDOWEVENT_CLOSE:
					printf ("SDL_WINDOWEVENT_CLOSE");
					break;
				default:
					break;
				}
				break;
				
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					event.type = SDL_QUIT;
					SDL_PushEvent (&event);
					break;
				default:
					break;
				}
				break;
				
			default:
				break;
			}
		}
		
		if (SDL_AtomicGet (&reader_atomic))
		{
			int x, y;
			if (SDL_GetMouseState(&x, &y) & SDL_BUTTON (SDL_BUTTON_LEFT))
			{
				int w, h;
				SDL_GetWindowSize(window,&w,&h);
				cv::circle (m_source, cv::Point2i (x*LEP3_W/w, y*LEP3_H/h), 6.0f, cv::Scalar (255), -1);
			}
			Subtractor->apply (m_source, m_fg);
			cv::GaussianBlur (m_fg, m_b, cv::Size (11, 11), 3.5, 3.5);
			cv::cvtColor (m_b, m_render, cv::COLOR_GRAY2BGR);
			Blobber->detect (m_b, Targets);
			
			cm_track (Targets, tracker.p, tracker.v, tracker.t, tracker.u, TRACKER_COUNT, tracker.proximity, tracker.persistence);
			bool counted = cm_countman (tracker.p, tracker.v, tracker.u, tracker.t, TRACKER_COUNT, way, tracker.persistence, tracker.confidence);
			if (counted) {cm_4way_print (way);}
	
			for (size_t i = 0; i < Targets.size (); i++)
			{
				cv::circle (m_render, Targets [i].pt, 6.0f, cv::Scalar (0, 255, 0), 1);
			}
			
			for (size_t i = 0; i < TRACKER_COUNT; i++)
			{
				char text [2];
				snprintf (text, 2, "%d", i);
				cv::putText (m_render, text, tracker.p [i] + cv::Point2f (-3.0f, 3.0f), cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 0.4, cv::Scalar (0, 0, 255), 1);	
				if (tracker.u [i] < tracker.persistence)
				{
					if (tracker.t [i] < tracker.confidence)
					{
						cv::circle (m_render, tracker.p [i], 6.0f, cv::Scalar (255, 0, 0), 1);
					}
					else
					{
						cv::circle (m_render, tracker.p [i], 6.0f, cv::Scalar (255, 100, 50), 1);
					}
				}
			}
			
			for (size_t i = 0; i < TRACKER_COUNT; i++)
			{
				if (tracker.t [i] != -1) {continue;}
				printf ("Tracker %i departured\n", i);
				//Reset target
				tracker.v [i] = {0.0f, 0.0f};
				tracker.p [i] = {(float)LEP3_W / 2.0f, (float)LEP3_H / 2.0f};
				tracker.t [i] = 0;
				tracker.u [i] = 0;
			}
			
			cv::rectangle (m_render, CM_N, cv::Scalar (255, 0, 255));
			cv::rectangle (m_render, CM_S, cv::Scalar (255, 0, 255));
			cv::rectangle (m_render, CM_W, cv::Scalar (255, 0, 255));
			cv::rectangle (m_render, CM_E, cv::Scalar (255, 0, 255));
			cv::rectangle (m_render, CM_NE, cv::Scalar (255, 0, 255));
			cv::rectangle (m_render, CM_NW, cv::Scalar (255, 0, 255));
			cv::rectangle (m_render, CM_SE, cv::Scalar (255, 0, 255));
			cv::rectangle (m_render, CM_SW, cv::Scalar (255, 0, 255));
		
			glBindTexture (GL_TEXTURE_2D, tex [0]);
			XGL_ASSERT_ERROR;
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, LEP3_W, LEP3_H, APP_RENDER_FORMAT, APP_RENDER_TYPE, m_render.ptr ());
			XGL_ASSERT_ERROR;
			glClear (GL_COLOR_BUFFER_BIT);XGL_ASSERT_ERROR;
			glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);XGL_ASSERT_ERROR;
			SDL_GL_SwapWindow (window);
			SDL_AtomicSet (&reader_atomic, 0);
		}
	}
	
main_quit:
	glUseProgram (0);
	glDisableVertexAttribArray (0);
	glDeleteProgram (program);
	glDeleteBuffers (1, &(ebo));
	glDeleteBuffers (1, &(vbo));
	SDL_GL_DeleteContext (context);
	SDL_DestroyWindow (window);
	SDL_Quit();
	return 0;
}
