#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <GL/glew.h>
#include "LoadShaders.h"

//----------------------------------------------------------------------------

GLchar* ReadShader(const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if (!fp) { fprintf(stderr, "cannot open %s\n", filename); return nullptr; }

	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	GLchar* src = static_cast<GLchar*>(malloc(len + 1));
	fread(src, 1, len, fp);
	fclose(fp);

	/* 式式 UTF-8 BOM 薯剪 式式 */
	if (len >= 3 &&
		static_cast<unsigned char>(src[0]) == 0xEF &&
		static_cast<unsigned char>(src[1]) == 0xBB &&
		static_cast<unsigned char>(src[2]) == 0xBF) {
		memmove(src, src + 3, len - 3);
		len -= 3;
	}
	src[len] = 0;
	return src;
}


//----------------------------------------------------------------------------

GLuint LoadShaders(ShaderInfo* shaders) {
	if (shaders == NULL) { return 0; }

	GLuint program = glCreateProgram();

	ShaderInfo* entry = shaders;
	while (entry->type != GL_NONE) {
		GLuint shader = glCreateShader(entry->type);

		entry->shader = shader;

		GLchar* source = ReadShader(entry->filename);
		if (source == NULL) {
			for (entry = shaders; entry->type != GL_NONE; ++entry) {
				glDeleteShader(entry->shader);
				entry->shader = 0;
			}

			return 0;
		}

		glShaderSource(shader, 1, &source, NULL);
		free(source);

		glCompileShader(shader);

		GLint compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
#ifdef _DEBUG
			GLsizei len;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

			GLchar* log = (GLchar*)malloc((len + 1) * sizeof(GLchar));
			glGetShaderInfoLog(shader, len, &len, log);
			fprintf(stdout, "Shader compilation failed: %s\n", log);
			free(log);
#endif /* DEBUG */

			return 0;
		}

		glAttachShader(program, shader);

		++entry;
	}

	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
#ifdef _DEBUG
		GLsizei len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		GLchar* log = (GLchar*)malloc((len + 1) * sizeof(GLchar));
		glGetProgramInfoLog(program, len, &len, log);
		fprintf(stdout, "Shader linking failed: %s\n", log);
		free(log);
#endif /* DEBUG */

		for (entry = shaders; entry->type != GL_NONE; ++entry) {
			glDeleteShader(entry->shader);
			entry->shader = 0;
		}

		return 0;
	}

	return program;
}


