#ifndef SHADERS_H
#define SHADERS_H

#include <GL/glew.h>

GLuint loadShader(GLenum type, const char *source);
GLuint loadShaderFromFile(GLenum type, const char *filename);
GLuint buildProgram(GLuint vertexShader, GLuint fragmentShader, const char* vertexPositionName);

#endif
