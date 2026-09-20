#include "opengl_debug.h"

namespace opengl_debug
{

namespace internal
{
GLuint samples_query = 0;
GLuint samples_passed = 0;
}  // namespace internal

void begin()
{
    glGenQueries(1, &internal::samples_query);
    // Initialize your buffers and textures ...
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    // Set up the values on the stencil buffer ...

    // Now we count the fragments that pass the stencil test
    glDepthFunc(GL_ALWAYS);  // Set up the depth test to always pass
    glBeginQuery(GL_SAMPLES_PASSED, internal::samples_query);
    // Render your meshes here
}

void end()
{
    glEndQuery(GL_SAMPLES_PASSED);
    glGetQueryObjectuiv(internal::samples_query, GL_QUERY_RESULT, &internal::samples_passed);

    glDeleteQueries(1, &internal::samples_query);
}

GLuint getSamplesPassed()
{
    return internal::samples_passed;
}

}  // namespace opengl_debug
