#include "zoom_rect.h"

#include <iostream>

using namespace lumos;

float rect_vertices[] = {
    -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,

    1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,

    1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
};

constexpr GLfloat xy_r = 1.0f;
constexpr GLfloat xy_g = 0.0f;
constexpr GLfloat xy_b = 0.0f;

constexpr GLfloat yz_r = 1.0f;
constexpr GLfloat yz_g = 0.0f;
constexpr GLfloat yz_b = 0.0f;

constexpr GLfloat xz_r = 1.0f;
constexpr GLfloat xz_g = 0.0f;
constexpr GLfloat xz_b = 0.0f;

GLfloat rect_color[] = {xy_r, xy_g, xy_b, xy_r, xy_g, xy_b, xy_r, xy_g, xy_b,

                        xy_r, xy_g, xy_b, xy_r, xy_g, xy_b, xy_r, xy_g, xy_b,

                        yz_r, yz_g, yz_b, yz_r, yz_g, yz_b, yz_r, yz_g, yz_b,

                        yz_r, yz_g, yz_b, yz_r, yz_g, yz_b, yz_r, yz_g, yz_b,

                        xz_r, xz_g, xz_b, xz_r, xz_g, xz_b, xz_r, xz_g, xz_b,

                        xz_r, xz_g, xz_b, xz_r, xz_g, xz_b, xz_r, xz_g, xz_b};

void ZoomRect::render(const Vec2f mouse_pos_at_press,
                      const Vec2f current_mouse_pos,
                      const SnappingAxis snapping_axis,
                      const glm::mat4& view_mat,
                      const glm::mat4& model_mat,
                      const glm::mat4& projection_mat)
{
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

    const Vec2f mouse_pos_at_press_mod =
        2.0f * (mouse_pos_at_press.elementWiseMultiply(Vec2f(1.0f, -1.0f)) + Vec2f(0.0f, 1.0f)) - 1.0f;
    const Vec2f current_mouse_pos_mod =
        2.0f * (current_mouse_pos.elementWiseMultiply(Vec2f(1.0f, -1.0f)) + Vec2f(0.0f, 1.0f)) - 1.0f;

    const glm::vec4 v_viewport = glm::vec4(-1, -1, 2, 2);

    glm::mat4 model_mat_local = model_mat;
    const glm::vec3 mouse_pos_at_press_projected(mouse_pos_at_press_mod.x, mouse_pos_at_press_mod.y, 0);
    const glm::vec3 current_mouse_pos_projected(current_mouse_pos_mod.x, current_mouse_pos_mod.y, 0);

    model_mat_local[3][0] = 0.0;
    model_mat_local[3][1] = 0.0;
    model_mat_local[3][2] = 0.0;

    const glm::mat4 view_model = view_mat * model_mat_local;

    const glm::vec3 current_mouse_pos_unprojected =
        glm::unProject(current_mouse_pos_projected, view_model, projection_mat, v_viewport);
    const glm::vec3 mouse_pos_at_press_unprojected =
        glm::unProject(mouse_pos_at_press_projected, view_model, projection_mat, v_viewport);

    if (SnappingAxis::SA_X == snapping_axis)
    {
        const float x_start = mouse_pos_at_press_unprojected.y;
        const float y_start = mouse_pos_at_press_unprojected.z;

        const float x_end = current_mouse_pos_unprojected.y;
        const float y_end = current_mouse_pos_unprojected.z;

        rect_vertices[0] = 0.0;
        rect_vertices[1] = x_start;
        rect_vertices[2] = y_start;

        rect_vertices[3] = 0.0;
        rect_vertices[4] = x_end;
        rect_vertices[5] = y_start;

        rect_vertices[6] = 0.0;
        rect_vertices[7] = x_end;
        rect_vertices[8] = y_end;

        rect_vertices[9] = 0.0;
        rect_vertices[10] = x_start;
        rect_vertices[11] = y_end;

        rect_vertices[12] = 0.0;
        rect_vertices[13] = x_start;
        rect_vertices[14] = y_start;
    }
    else if (SnappingAxis::SA_Y == snapping_axis)
    {
        const float x_start = mouse_pos_at_press_unprojected.x;
        const float y_start = mouse_pos_at_press_unprojected.z;

        const float x_end = current_mouse_pos_unprojected.x;
        const float y_end = current_mouse_pos_unprojected.z;

        rect_vertices[0] = x_start;
        rect_vertices[1] = 0.0;
        rect_vertices[2] = y_start;

        rect_vertices[3] = x_end;
        rect_vertices[4] = 0.0;
        rect_vertices[5] = y_start;

        rect_vertices[6] = x_end;
        rect_vertices[7] = 0.0;
        rect_vertices[8] = y_end;

        rect_vertices[9] = x_start;
        rect_vertices[10] = 0.0;
        rect_vertices[11] = y_end;

        rect_vertices[12] = x_start;
        rect_vertices[13] = 0.0;
        rect_vertices[14] = y_start;
    }
    else if (SnappingAxis::SA_Z == snapping_axis)
    {
        const float x_start = mouse_pos_at_press_unprojected.x;
        const float y_start = mouse_pos_at_press_unprojected.y;

        const float x_end = current_mouse_pos_unprojected.x;
        const float y_end = current_mouse_pos_unprojected.y;

        rect_vertices[0] = x_start;
        rect_vertices[1] = y_start;
        rect_vertices[2] = 0.0;

        rect_vertices[3] = x_end;
        rect_vertices[4] = y_start;
        rect_vertices[5] = 0.0;

        rect_vertices[6] = x_end;
        rect_vertices[7] = y_end;
        rect_vertices[8] = 0.0;

        rect_vertices[9] = x_start;
        rect_vertices[10] = y_end;
        rect_vertices[11] = 0.0;

        rect_vertices[12] = x_start;
        rect_vertices[13] = y_start;
        rect_vertices[14] = 0.0;
    }
    else
    {
        std::cout << "Shouldn't end up here!" << std::endl;
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * num_vertices_ * sizeof(float), rect_vertices);
    glBindVertexArray(vertex_buffer_array_);
    glDrawArrays(GL_LINE_STRIP, 0, num_vertices_);
    glBindVertexArray(0);
}

ZoomRect::ZoomRect()
{
    num_vertices_ = 5;
    const size_t num_bytes = sizeof(float) * 3 * num_vertices_;

    glGenVertexArrays(1, &vertex_buffer_array_);
    glBindVertexArray(vertex_buffer_array_);

    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, num_bytes, rect_vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &color_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, color_buffer_);
    glBufferData(GL_ARRAY_BUFFER, num_bytes, rect_color, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, color_buffer_);
    glVertexAttribPointer(1,         // attribute. No particular reason for 1, but must match the layout in the shader.
                          3,         // size
                          GL_FLOAT,  // type
                          GL_FALSE,  // normalized?
                          0,         // stride
                          (void*)0   // array buffer offset
    );
}
