#include "shaders.h"
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdio.h>

using namespace std;

GLuint loadShader(GLenum type, const char *source)
{
	//create a shader
	GLuint shader = glCreateShader(type);
	if (shader == 0)
	{
		cerr << "Error creating shader" << endl;
		return 0;
	}

	//load the shader source to the shader object and compile it
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	//check if the shader compiled successfully
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		cerr << "Shader compilation error for " << source << endl;
        GLsizei logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        GLsizei actualLogLength = 0;
        GLchar* buf = new GLchar[logLength];
        glGetShaderInfoLog(shader, logLength, &actualLogLength, buf);
        printf("%s\n", buf);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint loadShaderFromFile(GLenum type, const char* filename)
{
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cerr << "Error opening shader file: " << filename << "\n";
        return 0;
    }
    std::string str;
    std::stringstream ss;
    while (std::getline(ifs, str)) {
#ifndef EMSCRIPTEN
        if (str.size() > 9 && str.substr(0, 9) == "precision") {
            continue;
        }
#endif
        ss << str << "\n";
    }
    return loadShader(type, ss.str().c_str());
}

GLuint buildProgram(GLuint vertexShader, GLuint fragmentShader, const char* vertexPositionName)
{
	//create a GL program and link it
	GLuint programObject = glCreateProgram();
	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);
	glBindAttribLocation(programObject, 0, vertexPositionName);
	glLinkProgram(programObject);

	//check if the program linked successfully
	GLint linked;
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		cerr << "Program link error" << endl;
		glDeleteProgram(programObject);
		return 0;
	}
	return programObject;
}
