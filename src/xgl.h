#pragma once

#include "debug.h"
#include "tcol.h"
#include "malloc_file.h"

#define XGL_ASSERT_ERROR xgl_assert_error (__FILE__,__LINE__)
#define XGL_COLOR_SUCCESS TCOL (TCOL_NORMAL, TCOL_GREEN, TCOL_DEFAULT)
#define XGL_COLOR_ERROR TCOL (TCOL_NORMAL, TCOL_RED, TCOL_DEFAULT)
#define XGL_SHADER_NMAX 10
#define XGL_SHADER_NMAX 10


//Convert shader type to string.
char const * xgl_enum_str (GLenum type)
{
	switch (type)
	{
		case GL_VERTEX_SHADER:                  return "GL_VERTEX_SHADER";
		case GL_FRAGMENT_SHADER:                return "GL_FRAGMENT_SHADER";
		case GL_INVALID_OPERATION:              return "GL_INVALID_OPERATION";
		case GL_INVALID_ENUM:                   return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:                  return "GL_INVALID_VALUE";
		case GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:                  return "GL_OUT_OF_MEMORY";
		default: return "";
	}
	return "";
}


//Convert GL boolean to string.
char const * xgl_bool_str (GLint value)
{
	switch (value)
	{
		case GL_TRUE:return "GL_TRUE";
		case GL_FALSE:return "GL_FALSE";
		default: return "";
	}
	return "";
}


void xgl_assert_error (const char *file, int line)
{
	GLenum e = glGetError ();
	if (e == GL_NO_ERROR) {return;}
	while (1)
	{
		fprintf (stderr, "%s:%i %s (%i)", file, line, xgl_enum_str (e), e);
		e = glGetError ();
		if (e == GL_NO_ERROR) {break;}
	}
	exit (1);
}


void xgl_uniform1i_set (GLuint program, GLchar const * name, GLint v0)
{
	GLint location;
	location = glGetUniformLocation (program, name);
	ASSERT (location >= 0);
	glUniform1i (location, v0);
}


void xgl_program_free_shaders (GLuint program)
{
	ASSERT (glIsProgram (program));
	GLsizei i = XGL_SHADER_NMAX;
	GLuint shaders [XGL_SHADER_NMAX];
	glGetAttachedShaders (program, i, &i, shaders);
	while (i--)
	{
		glDetachShader (program, shaders [i]);
		glDeleteShader (shaders [i]);
	}
}


void xgl_attach_shaderfile (GLuint program, char const * filename, GLenum kind)
{
	ASSERT (kind != 0);
	GLuint shader = glCreateShader (kind);
	XGL_ASSERT_ERROR;
	ASSERT (shader != 0);
	//Set the shader source code:
	char * buffer = malloc_file (filename);
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-qual"
	glShaderSource (shader, 1, (const GLchar **) &buffer, NULL);
	#pragma GCC diagnostic pop
	XGL_ASSERT_ERROR;
	free (buffer);
	glCompileShader (shader);
	glAttachShader (program, shader);
	//If a shader object to be deleted is attached to a program object, it will be flagged for deletion, 
	//but it will not be deleted until it is no longer attached to any program object, 
	//for any rendering context (i.e., it must be detached from wherever it was attached before it will be deleted).
	glDeleteShader (shader);
	XGL_ASSERT_ERROR;
}


struct xgl_program_info
{
	GLint type;
	GLint delete_status;
	GLint compile_status;
	GLint infolog_length;
	GLint source_length;
	char * log;
};


void xgl_program_info_get (GLuint shader, struct xgl_program_info * info)
{
	glGetShaderiv (shader, GL_SHADER_TYPE, &info->type);
	glGetShaderiv (shader, GL_DELETE_STATUS, &info->delete_status);
	glGetShaderiv (shader, GL_COMPILE_STATUS, &info->compile_status);
	glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info->infolog_length);
	glGetShaderiv (shader, GL_SHADER_SOURCE_LENGTH, &info->source_length);
	GLsizei l = info->infolog_length;
	info->log = (char *) malloc (sizeof (char) * (size_t)l);
	glGetShaderInfoLog (shader, l, &l, info->log);
}


void xgl_program_print (GLuint program)
{
	FILE * f = stderr;
	GLsizei n = XGL_SHADER_NMAX;
	GLuint shaders [XGL_SHADER_NMAX];
	glGetAttachedShaders (program, n, &n, shaders);
	fprintf (f, "\u250C");
	fprintf (f, "%8s %8s %20s %10s %10s %10s %10s\n", "PROGRAM", "SHADER", "TYPE", "DELETE", "COMPILE", "LOGLEN", "SRCLEN");
	for (GLsizei i = 0; i < n; ++ i)
	{
		struct xgl_program_info info;
		xgl_program_info_get (shaders [i], &info);
		fprintf (f, "\u251C");
		fprintf (f, "%8i ", (int) program);
		fprintf (f, "%8i ", (int) shaders [i]);
		fprintf (f, "%20s " , xgl_enum_str ((GLenum)info.type));
		if (info.delete_status) {fprintf (f, XGL_COLOR_SUCCESS "%10s " TCOL_RESET, xgl_bool_str (info.delete_status));}
		else {fprintf (f, XGL_COLOR_ERROR "%10s " TCOL_RESET, xgl_bool_str (info.delete_status));}
		if (info.compile_status) {fprintf (f, XGL_COLOR_SUCCESS "%10s " TCOL_RESET, xgl_bool_str (info.compile_status));}
		else {fprintf (f, XGL_COLOR_ERROR "%10s " TCOL_RESET, xgl_bool_str (info.compile_status));}
		fprintf (f, "%10i ", (int) info.infolog_length);
		fprintf (f, "%10i ", (int) info.source_length);
		fprintf (f, "%.*s", info.infolog_length, info.log);
		fprintf (f, "\n");
	}
	fflush (f);
}


void xglVertexAttribPointer
(
	GLuint program,
	const GLchar *name,
	GLint size,
	GLenum type,
	GLboolean normalized,
	GLsizei stride,
	const GLvoid * pointer
)
{
	GLint loc = glGetAttribLocation (program, name);
	ASSERT (loc >= 0);
	printf ("loc %i \n", loc);
	glVertexAttribPointer ((GLuint)loc, size, type, normalized, stride, pointer);
	glEnableVertexAttribArray ((GLuint)loc);
	XGL_ASSERT_ERROR;
}