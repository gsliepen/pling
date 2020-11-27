/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

class ShaderProgram
{
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;

public:
	ShaderProgram(const char *vertex_source, const char *fragment_source);
	~ShaderProgram();

	void use() const
	{
		glUseProgram(program);
	};

	GLint get_uniform(const char *name) const
	{
		return glGetUniformLocation(program, name);
	}

	GLint get_attrib(const char *name) const
	{
		return glGetAttribLocation(program, name);
	}
};
