/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "shader.hpp"

#include <fmt/ostream.h>
#include <iostream>
#include <stdexcept>

static void print_log(GLuint object) {
	GLint log_length;

	if (glIsShader(object)) {
		glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
	} else if (glIsProgram(object)) {
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
	} else {
		fmt::print(std::cerr, "Not a shader or a program\n");
		return;
	}

	std::string log;
	log.resize(log_length);

	if (glIsShader(object)) {
		glGetShaderInfoLog(object, log.size(), nullptr, log.data());
	} else if (glIsProgram(object)) {
		glGetProgramInfoLog(object, log.size(), nullptr, log.data());
	}

	fmt::print(std::cerr, "{}\n", log);
}

static GLuint compile(GLenum type, const char *source) {
	GLuint shader = glCreateShader(type);
	if (!shader)
		throw std::runtime_error("Error creating shader");

	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint ok = false;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		print_log(shader);
		throw std::runtime_error("Error compiling shader");
	}

	return shader;
}

static GLuint link(GLuint vertex_shader, GLuint fragment_shader) {
	GLuint program = glCreateProgram();
	if (!program)
		throw std::runtime_error("Error creating shader program");

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	GLint ok = false;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		print_log(program);
		throw std::runtime_error("Error linking shader program");
	}

	return program;
}

ShaderProgram::ShaderProgram(const char *vertex_source, const char *fragment_source):
	vertex_shader(compile(GL_VERTEX_SHADER, vertex_source)),
	fragment_shader(compile(GL_FRAGMENT_SHADER, fragment_source)),
	program(link(vertex_shader, fragment_shader))
{
}

ShaderProgram::~ShaderProgram() {
	glDeleteProgram(program);
	glDeleteShader(fragment_shader);
	glDeleteShader(vertex_shader);
}
