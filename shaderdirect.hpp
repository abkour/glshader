#pragma once
#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using shader_p = std::pair<GLenum, std::string>;

struct ShaderWrapper {

	ShaderWrapper() noexcept : programID(0) {}

	template<typename... T, typename = std::enable_if<std::conjunction_v<std::is_same<std::pair<GLenum, std::string>, T>...>>>
	ShaderWrapper(bool enableExtendedGLSL, T&&... args) {
		std::vector<typename std::common_type<T...>::type> shaders = { args... };
		std::vector<GLuint> shaderIds(shaders.size());
		shaderIds.reserve(shaders.size());
		
		for (auto shader : shaders) {
			shaderIds.push_back(glCreateShader(std::get<GLenum>(shader)));

			std::ifstream shaderFile(std::get<std::string>(shader));
			if (shaderFile.fail()) {
				throw std::runtime_error("Filename " + std::get<std::string>(shader) + " does not exist!");
			}

			std::stringstream shaderSource;
			shaderSource << shaderFile.rdbuf();
			shaderFile.close();

			auto shaderSourceString = shaderSource.str();
			// Parse the source code for extended GLSL features
			if (enableExtendedGLSL) {
				parseSource(shaderSourceString);
			}

			const auto shaderSourceCString = shaderSourceString.c_str();

			glShaderSource(shaderIds.back(), 1, &shaderSourceCString, NULL);
			glCompileShader(shaderIds.back());

			isShaderCompilationValid(std::get<GLenum>(shader), shaderIds.back(), shaderIds);
		}
		
		programID = glCreateProgram();
		for (const auto& shaderID : shaderIds) { glAttachShader(programID, shaderID); }
		glLinkProgram(programID);
		for (const auto& shaderID : shaderIds) { glDetachShader(programID, shaderID); }

		isProgramLinkageValid(programID, shaderIds);
		for (const auto& shaderID : shaderIds) { glDetachShader(programID, shaderID); }
	}

	inline ShaderWrapper(ShaderWrapper&& other) noexcept
		: programID(std::exchange(other.programID, 0))
	{}

	inline ShaderWrapper& operator=(ShaderWrapper&& other) noexcept {
		programID = std::exchange(other.programID, 0);
		return *this;
	}

	inline ~ShaderWrapper() {
		glDeleteProgram(programID);
	}

	inline void bind() {
		glUseProgram(programID);
	}

	inline GLuint id() const {
		return programID;
	}

	// You never want to create a copy of a shader. Give me one good reason.
	ShaderWrapper(const ShaderWrapper& other) = delete;
	inline ShaderWrapper& operator=(const ShaderWrapper& other) = delete;

private:

	GLuint programID;

	static inline bool isShaderCompilationValid(GLenum shaderType, GLuint shaderID, std::vector<GLuint>& shaderIds);
	static inline void isProgramLinkageValid(GLuint programID, std::vector<GLuint>& shaderIds);

	void parseSource(std::string& source);
};

bool ShaderWrapper::isShaderCompilationValid(GLenum shaderType, GLuint shaderID, std::vector<GLuint>& shaderIds) {
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
		for (const auto& shaderID : shaderIds) { glDeleteShader(shaderID); }
		throw std::runtime_error(errorMessage + "FAILED_COMPILATION. ERROR MESSAGE: " + std::string(errorLog));
	}
}

void ShaderWrapper::isProgramLinkageValid(GLuint programID, std::vector<GLuint>& shaderIds) {
	int success;
	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (success != GL_TRUE) {
		char errorLog[512];
		glGetShaderInfoLog(programID, 512, NULL, errorLog);
		for (const auto& shaderID : shaderIds) { glDeleteShader(shaderID); }
		std::string errorMessage = "Program linkage error. Error message: " + std::string(errorLog);
		throw std::runtime_error(errorMessage);
	}
}

inline void ShaderWrapper::parseSource(std::string& source) {
	std::size_t nextTokenPosition = 0;
	std::size_t positionOfRoute = 0;
	std::size_t linePositionOfDirective = 0;
	std::size_t currentLine = 0;
	std::size_t linePositionOfLastComment = 0;
	constexpr char include_str[] = "include <";
	constexpr std::size_t include_size = sizeof(include_str) - 1;
	while (true) {
		// 1. Find the first # token
		bool characterFound = false;
		for (int i = nextTokenPosition; i < source.size(); ++i) {
			if (source[i] == '#' && linePositionOfLastComment != currentLine) {
				nextTokenPosition = i + 1; // Increment by 1 avoids circular loops.
				positionOfRoute = i;
				linePositionOfDirective = currentLine;
				characterFound = true;
				break;
			} else if (source[i] == '\n') {
				currentLine++;
			} else if (source[i] == '/') {
				// Am I allowed to look at the next character? Yes, if it's not past the last
				if (i + 1 < source.size()) {
					if (source[i + 1] == '/') {
						linePositionOfLastComment = currentLine;
						i++;
					}
				}
			}
		}

		if (!characterFound) {
			break;
		}

		// Do the following characters match "include <"? If no, continue with the next token.
		if (source.compare(nextTokenPosition, include_size, include_str) != 0) {
			continue;
		}

		nextTokenPosition += include_size;
		auto prevTokenPosition = nextTokenPosition;

		// 3. Find the closing token '>' that is followed by '<' on the same line
		for (int i = nextTokenPosition + 1; i < source.size(); ++i) {
			if (source[i] == '>') {
				nextTokenPosition = i;
				break;
			} else if (source[i] == '\n') {
				throw std::runtime_error("Error. Could not find end of #include statement!");
			}
		}

		const auto linesize = (nextTokenPosition - positionOfRoute) + 1;
		const auto include_path = source.substr(prevTokenPosition, nextTokenPosition - prevTokenPosition);
		source.erase(positionOfRoute, linesize);
		nextTokenPosition -= linesize;
		
		// Insert the code from the include path to the source code
		std::ifstream include_file(include_path, std::ios::binary);
		if (include_file.fail()) {
			throw std::runtime_error("Could not opne file: " + include_path);
		}

		std::stringstream include_file_stream;
		include_file_stream << include_file.rdbuf();
		include_file.close();

		auto include_file_source = include_file_stream.str();
		source.insert(nextTokenPosition + 1, include_file_source.c_str(), include_file_source.size());
		nextTokenPosition += include_file_source.size();
	}
}