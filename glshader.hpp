#pragma once
#include <glad/glad.h>
#include <string_view>

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace tinygui {

struct Shader {

	// This is only used as an intermediate step.
	inline Shader() noexcept;
	inline Shader(Shader&& other) noexcept;

	inline Shader(std::initializer_list<std::pair<GLenum, std::string>>&& shaders_list);

	inline Shader(const char* vertexshaderPath, const char* fragmentshaderPath);
	inline ~Shader();

	// You never want to create a copy of shader. Give me one good reason.
	inline Shader& operator=(const Shader& other) = delete;
	inline Shader operator=(Shader&& other) = delete;

	inline void bind();
	inline GLuint id() const {
		return programID;
	}

private:

	GLuint programID;

private:

	static inline bool isShaderCompilationValid(GLenum shaderType, GLuint shaderID, std::vector<GLuint>& shaderIds);
	static inline void isProgramLinkageValid(GLuint programID, std::vector<GLuint>& shaderIds);
};

Shader::Shader() noexcept
	: programID(0)
{}

Shader::Shader(std::initializer_list<std::pair<GLenum, std::string>>&& shaders_list) {
	std::vector<std::pair<GLenum, std::string>> shaders(shaders_list);
	std::vector<GLuint> shaderIds(shaders.size());
	shaderIds.reserve(shaders.size());

	for (const auto& shader : shaders) {
		shaderIds.push_back(glCreateShader(std::get<GLenum>(shader)));

		std::ifstream shaderFile(std::get<std::string>(shader));
		if (shaderFile.fail()) {
			throw std::runtime_error("Filename " + std::get<std::string>(shader) + " does not exist!");
		}

		std::stringstream shaderSource;
		shaderSource << shaderFile.rdbuf();
		shaderFile.close();

		const auto shaderSourceString = shaderSource.str();
		const auto shaderSourceCString = shaderSourceString.c_str();

		glShaderSource(shaderIds.back(), 1, &shaderSourceCString, NULL);
		glCompileShader(shaderIds.back());

		isShaderCompilationValid(std::get<GLenum>(shader), shaderIds.back(), shaderIds);
	}

	programID = glCreateProgram();
	std::for_each(shaderIds.begin(), shaderIds.end(), [&](GLuint shaderId) { glAttachShader(programID, shaderId); });
	glLinkProgram(programID);
	std::for_each(shaderIds.begin(), shaderIds.end(), [&](GLuint shaderId) { glDetachShader(programID, shaderId); });

	isProgramLinkageValid(programID, shaderIds);
	std::for_each(shaderIds.begin(), shaderIds.end(), [](GLuint shaderId) { glDeleteShader(shaderId); });
}

Shader::Shader(Shader&& other) noexcept {
	programID = other.programID;
	other.programID = 0;
}

Shader::~Shader() {
	glDeleteProgram(programID);
}

void Shader::bind() {
	glUseProgram(programID);
}

bool Shader::isShaderCompilationValid(GLenum shaderType, GLuint shaderID, std::vector<GLuint>& shaderIds) {
	int success;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		char errorLog[512];
		std::string errorMessage;
		glGetShaderInfoLog(shaderID, 512, NULL, errorLog);
		switch (shaderType) {
		case GL_VERTEX_SHADER:
			errorMessage += "VERTEX_SHADER::";
			break;
		case GL_TESS_CONTROL_SHADER:
			errorMessage += "TESSELLATION_CONTROL_SHADER::";
			break;
		case GL_TESS_EVALUATION_SHADER:
			errorMessage += "TESSELLATION_EVALUATION_SHADER::";
			break;
		case GL_GEOMETRY_SHADER:
			errorMessage += "GEOMETRY_SHADER::";
			break;
		case GL_FRAGMENT_SHADER:
			errorMessage += "FRAGMENT_SHADER::";
			break;
		case GL_COMPUTE_SHADER:
			errorMessage += "COMPUTE_SHADER::";
			break;
		default:
			errorMessage += "INCORRECT_SHADER_SPECIFIED::";
			break;
		}
		std::for_each(shaderIds.begin(), shaderIds.end(), [](GLuint shaderId) { glDeleteShader(shaderId); });
		throw std::runtime_error(errorMessage + "FAILED_COMPILATION. ERROR MESSAGE: " + std::string(errorLog));
	}
}

void Shader::isProgramLinkageValid(GLuint programID, std::vector<GLuint>& shaderIds) {
	int success;
	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (success != GL_TRUE) {
		char errorLog[512];
		glGetShaderInfoLog(programID, 512, NULL, errorLog);
		
		std::for_each(shaderIds.begin(), shaderIds.end(), [](GLuint shaderId) { glDeleteShader(shaderId); });
		
		std::string errorMessage = "Program linkage error. Error message: " + std::string(errorLog);
		throw std::runtime_error(errorMessage);
	}
}

}