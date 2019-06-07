#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#include <stdio.h>
#include <unistd.h> //getopt
#include <vector>

#include <opencv4/opencv2/core/core.hpp>
#include <opencv4/opencv2/core/utility.hpp>
#include <opencv4/opencv2/imgproc/imgproc.hpp>
#include <opencv4/opencv2/features2d/features2d.hpp>
#include <opencv4/opencv2/video/background_segm.hpp>

#include "debug.h"
#include "xgl.h"
#include "lepton.h"
#include "threadcap.h"
#include "cm.hpp"
#include "shared.hpp"

#define APP_TEX_COUNT 1
#define APP_VISUAL_C          1
#define APP_VISUAL_TYPE       GL_UNSIGNED_BYTE
#define APP_VISUAL_FORMAT     GL_RGB
#define APP_VISUAL_INTFORMAT  GL_RGB
#define APP_VISUAL_UNIT       0
#define APP_VISUAL_MAG_FILTER GL_NEAREST
//#define APP_VISUAL_MAG_FILTER GL_LINEAR
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
	
	program = glCreateProgram ();
	xgl_attach_shaderfile (program, APP_SHADERF, GL_FRAGMENT_SHADER);
	xgl_attach_shaderfile (program, APP_SHADERV, GL_VERTEX_SHADER);
	glLinkProgram (program);
	glUseProgram (program);
	xgl_program_print (program);
	
	float vertices [] = 
	{
		//x      y     s      t
		-1.0f, -1.0f, 0.0f,  1.0f, // BL
		-1.0f,  1.0f, 0.0f,  0.0f, // TL
		 1.0f,  1.0f, 1.0f,  0.0f, // TR
		 1.0f, -1.0f, 1.0f,  1.0f  // BR
	};
	const GLint indicies [] = {0, 1, 2, 0, 2, 3};
	
	glGenBuffers (1, &(vbo));
	glBindBuffer (GL_ARRAY_BUFFER, vbo);
	glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
	glGenBuffers (1, &(ebo));
	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indicies), indicies, GL_STATIC_DRAW);
	xglVertexAttribPointer (program, "pos", 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
	glUseProgram (program);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	//Setup OpenGL texture
	GLuint tex [APP_TEX_COUNT];
	glGenTextures (APP_TEX_COUNT, tex);
	glActiveTexture (GL_TEXTURE0 + APP_VISUAL_UNIT);
	glBindTexture (GL_TEXTURE_2D, tex [APP_VISUAL_UNIT]);
	xgl_uniform1i_set (program, "tex", APP_VISUAL_UNIT);
	//glPixelStorei(GL_UNPACK_ALIGNMENT, (M2.step & 3) ? 1 : 4);
	glTexImage2D (GL_TEXTURE_2D, 0, APP_VISUAL_INTFORMAT, LEP3_W, LEP3_H, 0, APP_VISUAL_FORMAT, APP_VISUAL_TYPE, NULL);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, APP_VISUAL_MAG_FILTER);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
	glGenerateMipmap (GL_TEXTURE_2D);
	
	
	cv::Ptr<cv::BackgroundSubtractor> subtractor;
	cv::Ptr<cv::SimpleBlobDetector> blobber;
	cv::Mat mat_source (LEP3_H, LEP3_W, CV_16U);
	cv::Mat mat_fg (LEP3_H, LEP3_W, CV_8U);
	cv::Mat mat_b (LEP3_H, LEP3_W, CV_8U);
	cv::Mat mat_visual (LEP3_H, LEP3_W, CV_8UC3);
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
	
	threadcap_start (mat_source.ptr ());
	
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
		
		if (SDL_AtomicGet (&threadcap_atomic))
		{
			int x, y;
			if (SDL_GetMouseState(&x, &y) & SDL_BUTTON (SDL_BUTTON_LEFT))
			{
				int w, h;
				SDL_GetWindowSize(window,&w,&h);
				cv::circle (mat_source, cv::Point2i (x*LEP3_W/w, y*LEP3_H/h), 6.0f, cv::Scalar (255), -1);
			}
			
			sfilter
			(
				subtractor, //background subtraction
				blobber, //blob detection
				mat_source, //camera source raw input
				mat_fg, //foreground of background subtraction
				mat_b, //blurred image
				kp, //target keypoints
				tracker, //
				way
			);
			
			if (way.counted > 0) {cm_4way_print (way);}
			
			cv::cvtColor (mat_b, mat_visual, cv::COLOR_GRAY2BGR);
			
			for (size_t i = 0; i < kp.size (); i++)
			{
				cv::drawMarker (mat_visual, kp [i].pt, cv::Scalar (200, 100, 0), cv::MARKER_CROSS, 6);
			}
			
			for (size_t i = 0; i < STRACKER_COUNT; i++)
			{
				char text [12];
				snprintf (text, 12, "%d %d %d", i, tracker.t [i], tracker.u [i]);
				cv::putText (mat_visual, text, tracker.p [i] + cv::Point2f (-3.0f, 3.0f), cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 0.3, cv::Scalar (0, 0, 255), 1);	
				if (tracker.u [i] < STRACKER_PERSISTENCE)
				{
					//Draw velocity vector (dx, dy) line.
					cv::line (mat_visual, tracker.p [i], tracker.p [i] + (tracker.v [i] * 30.0f), cv::Scalar (0, 255, 100));
					if (tracker.t [i] < STRACKER_BORDER_CONFIDENCE)
					{
						//This tracker can not depart.
						cv::circle (mat_visual, tracker.p [i], STRACKER_PROXIMITY, cv::Scalar (44, 0, 0), 1);
					}
					else
					{
						//This tracker might depart.
						cv::circle (mat_visual, tracker.p [i], STRACKER_PROXIMITY, cv::Scalar (0, 100, 10), 1);
					}
				}
			}
			
			cv::rectangle (mat_visual, CM_N, cv::Scalar (255, 0, 255));
			cv::rectangle (mat_visual, CM_S, cv::Scalar (255, 0, 255));
			cv::rectangle (mat_visual, CM_W, cv::Scalar (255, 0, 255));
			cv::rectangle (mat_visual, CM_E, cv::Scalar (255, 0, 255));
			cv::rectangle (mat_visual, CM_NE, cv::Scalar (255, 0, 255));
			cv::rectangle (mat_visual, CM_NW, cv::Scalar (255, 0, 255));
			cv::rectangle (mat_visual, CM_SE, cv::Scalar (255, 0, 255));
			cv::rectangle (mat_visual, CM_SW, cv::Scalar (255, 0, 255));
		
			glBindTexture (GL_TEXTURE_2D, tex [0]);
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, LEP3_W, LEP3_H, APP_VISUAL_FORMAT, APP_VISUAL_TYPE, mat_visual.ptr ());
			glClear (GL_COLOR_BUFFER_BIT);
			glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
			SDL_GL_SwapWindow (window);
			SDL_AtomicSet (&threadcap_atomic, 0);
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
