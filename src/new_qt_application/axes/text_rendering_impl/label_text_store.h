#pragma once
#include <string>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "buffers/gBuffers.h"
#include "font_atlas.h"

struct label_text
{
    std::string label;
    glm::vec2   label_loc;
    glm::vec3   label_color;
    float       label_angle;
    float       label_x_size;
    float       label_y_size;
};

class label_text_store
{
public:
    label_text_store() = default;
    ~label_text_store() = default;

    // Load font and prepare for shaping. Call before add_text.
    void init(const std::string& font_path, uint32_t pixel_size = 128);

    void add_text(const std::string& label, glm::vec2 text_loc, glm::vec3 text_color,
                  float font_angle, float x_size, float y_size);

    // Build GPU buffers from all added labels (call after all add_text calls).
    void set_buffers();

    void paint_text();

    // Public for access by text_rendering.cpp
    font_atlas              main_font;
    std::vector<label_text> labels;

private:
    gBuffers             label_buffers;
    uint32_t             total_glyph_count = 0;

    uint32_t get_buffer(const label_text& lb,
                        float* vertices, uint32_t& vertex_index,
                        uint32_t* indices, uint32_t& indices_index);

    glm::vec2 rotate_pt(const glm::vec2& rotate_about, const glm::vec2& pt, float rotation_angle);
};
