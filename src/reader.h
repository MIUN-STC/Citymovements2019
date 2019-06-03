#pragma once
#include <SDL2/SDL.h>
#include "lepton.h"
#include "debug.h"

SDL_atomic_t reader_atomic;
SDL_Thread * reader_thread = NULL;
int reader (void * ptr)
{
	while (1)
	{
		uint16_t img [LEP3_WH];
		int r = fread (img, sizeof (img), 1, stdin);
		//ASSERT_F (r == 1, "fread () %i", r);
		if (r == 1 && SDL_AtomicGet (&reader_atomic) == 0)
		{
			memcpy (ptr, img, sizeof (img));
			SDL_AtomicAdd (&reader_atomic, 1);
		}
	}
	return 0;
}


void reader_start (void * ptr)
{
	reader_thread = SDL_CreateThread (reader, "reader", (void *)ptr);
	ASSERT_F (reader_thread, "SDL_CreateThread: %s", SDL_GetError());
}
