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
		int r = read (STDIN_FILENO, img, sizeof(img));
		ASSERT_F (r == sizeof(img), "read () error. Read %d of %d", r, sizeof(img));
		if (SDL_AtomicGet (&reader_atomic) == 0)
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