#include <stdlib.h>
#include <stdio.h>
#include "debug.h"


char * malloc_file (char const * filename)
{
	ASSERT_F (filename != NULL, "filename is NULL%s", "");
	FILE * file = fopen (filename, "rb");
	ASSERT_F (file != NULL, "file is NULL%s", "");
	fseek (file, 0, SEEK_END);
	long int length = ftell (file);
	fseek (file, 0, SEEK_SET);
	char * buffer = (char *) malloc ((size_t)length + 1);
	ASSERT_F (buffer != NULL, "buffer is NULL%s", "");
	memset (buffer, 0, (size_t)length + 1);
	//buffer [length + 1] = 0;
	{
		size_t r = fread (buffer, (size_t)length, 1, file);
		ASSERT_F (r == 1, "fread error %i %i", (int)r, (int)length);
	}
	fclose (file);
	return buffer;
}