#pragma once
#include <SDL2/SDL.h>
#include "lepton.h"
#include "debug.h"

SDL_atomic_t threadcap_atomic;
SDL_Thread * threadcap_thread = NULL;

int threadcap_read (void * ptr)
{
	while (1)
	{
		uint16_t img [LEP3_WH];
		int r = fread (img, sizeof (img), 1, stdin);
		//ASSERT_F (r == 1, "fread () %i", r);
		if (r == 1 && SDL_AtomicGet (&threadcap_atomic) == 0)
		{
			memcpy (ptr, img, sizeof (img));
			SDL_AtomicAdd (&threadcap_atomic, 1);
		}
	}
	return 0;
}

void threadcap_start (void * ptr)
{
	threadcap_thread = SDL_CreateThread (threadcap_read, "threadcap_read", (void *)ptr);
	ASSERT_F (threadcap_thread, "SDL_CreateThread: %s", SDL_GetError());
}
