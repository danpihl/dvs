#ifndef MAIN_APPLICATION_OPENGL_LOW_LEVEL_OPENGL_HEADER_H_
#define MAIN_APPLICATION_OPENGL_LOW_LEVEL_OPENGL_HEADER_H_

#ifdef PLATFORM_LINUX_M
// clang-format off
// GL_GLEXT_PROTOTYPES is defined globally in CMakeLists (linux.cmake) so that
// gl.h's internal include of glext.h already sees it and declares all extension
// function prototypes (glGenVertexArrays, glUniform*, etc.).
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
// clang-format on
#endif

#ifdef PLATFORM_APPLE_M

// #include <GLUT/glut.h>
#include <OpenGL/gl3.h>
// #include <OpenGl/glu3.h>

#endif

#endif  // MAIN_APPLICATION_OPENGL_LOW_LEVEL_OPENGL_HEADER_H_
