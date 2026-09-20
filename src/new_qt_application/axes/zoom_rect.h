#ifndef MAIN_APPLICATION_AXES_ZOOM_RECT_H_
#define MAIN_APPLICATION_AXES_ZOOM_RECT_H_

#include <stddef.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "axes/structures/axes_limits.h"
#include "axes/structures/view_angles.h"
#include "lumos/math.h"
#include "opengl_low_level/opengl_header.h"

class ZoomRect
{
private:
    GLuint vertex_buffer_, vertex_buffer_array_, color_buffer_;
    size_t num_vertices_;

public:
    ZoomRect();
    void render(const lumos::Vec2f mouse_pos_at_press,
                const lumos::Vec2f current_mouse_pos,
                const SnappingAxis snapping_axis,
                const glm::mat4& view_mat,
                const glm::mat4& model_mat,
                const glm::mat4& projection_mat);
};

#endif  // MAIN_APPLICATION_AXES_ZOOM_RECT_H_
