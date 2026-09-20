#ifndef MAIN_APPLICATION_OPENGL_DEBUG_H_
#define MAIN_APPLICATION_OPENGL_DEBUG_H_

#include "opengl_low_level/opengl_header.h"

namespace opengl_debug
{

void begin();
void end();
GLuint getSamplesPassed();

}  // namespace opengl_debug

#endif  // MAIN_APPLICATION_OPENGL_DEBUG_H_
