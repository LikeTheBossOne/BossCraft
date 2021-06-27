#pragma once
#include <string>
#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:
	unsigned int id;

	Shader(const char* vertexPath, const char* fragmentPath)
	{
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file’s buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}

		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();


		// Compile Shaders
		unsigned int vertex, fragment;
		int success;
		char infoLog[512];

		// vertex Shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);

		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" <<
				infoLog << std::endl;
		}

		// fragment Shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);

		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" <<
				infoLog << std::endl;
		}


		id = glCreateProgram();
		glAttachShader(id, vertex);
		glAttachShader(id, fragment);
		glLinkProgram(id);
		// print linking errors if any
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(id, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" <<
				infoLog << std::endl;
		}

		// Delete Shaders
		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	void Use()
	{
		glUseProgram(id);
	}

	void UniSetBool(const std::string& name, bool value)
	{
		glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
	}

	void UniSetInt(const std::string& name, int value)
	{
		glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
	}

	void UniSetFloat(const std::string& name, float value)
	{
		glUniform1f(glGetUniformLocation(id, name.c_str()), value);
	}

	void UniSetMat4f(const std::string& name, glm::mat4 value)
	{
		glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
	}
};
