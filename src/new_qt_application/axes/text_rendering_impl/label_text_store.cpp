#include "label_text_store.h"
#include <cmath>
#include <set>
#include <hb.h>

void label_text_store::init(const std::string& font_path, uint32_t pixel_size)
{
    labels.clear();
    total_glyph_count = 0;
    main_font.init(font_path, pixel_size);
}

void label_text_store::add_text(const std::string& label, glm::vec2 label_loc, glm::vec3 label_color,
    float label_angle, float x_size, float y_size)
{
    label_text t;
    t.label        = label;
    t.label_loc    = label_loc;
    t.label_color  = label_color;
    t.label_angle  = label_angle;
    t.label_x_size = x_size;
    t.label_y_size = y_size;
    labels.push_back(t);
}

void label_text_store::set_buffers()
{
    // Phase 1: shape every label to collect all glyph IDs.
    std::set<uint32_t> glyph_id_set;
    uint32_t max_chars = 0;
    for (const auto& lb : labels) {
        hb_buffer_t* hb_buf = hb_buffer_create();
        hb_buffer_add_utf8(hb_buf, lb.label.c_str(), -1, 0, -1);
        hb_buffer_guess_segment_properties(hb_buf);
        hb_shape(main_font.hb_font, hb_buf, nullptr, 0);

        unsigned int     glyph_count = 0;
        hb_glyph_info_t* glyph_info  = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);
        for (unsigned int i = 0; i < glyph_count; i++) {
            glyph_id_set.insert(glyph_info[i].codepoint);
        }
        max_chars += glyph_count;
        hb_buffer_destroy(hb_buf);
    }

    // Phase 2: build the GPU atlas for exactly those glyphs.
    std::vector<uint32_t> glyph_ids(glyph_id_set.begin(), glyph_id_set.end());
    main_font.build_atlas(glyph_ids);

    // Phase 3: build vertex/index buffers.
    // Vertex layout: vec2 pos, vec2 origin, vec2 uv, vec3 color = 9 floats, 4 vertices per glyph.
    const uint32_t floats_per_vertex = 9;
    const uint32_t verts_per_glyph   = 4;
    const uint32_t idx_per_glyph     = 6;

    float*    vertices = new float[max_chars * verts_per_glyph * floats_per_vertex];
    uint32_t* indices  = new uint32_t[max_chars * idx_per_glyph];

    uint32_t v_index = 0;
    uint32_t i_index = 0;
    uint32_t actual_glyphs = 0;

    for (const auto& lb : labels) {
        actual_glyphs += get_buffer(lb, vertices, v_index, indices, i_index);
    }
    total_glyph_count = actual_glyphs;

    VertexBufferLayout layout;
    layout.AddFloat(2);  // position
    layout.AddFloat(2);  // origin (for rotation)
    layout.AddFloat(2);  // UV coords
    layout.AddFloat(3);  // color

    uint32_t vb_size = v_index * static_cast<uint32_t>(sizeof(float));
    label_buffers.CreateBuffers(vertices, vb_size, indices, i_index, layout);

    delete[] vertices;
    delete[] indices;
}

void label_text_store::paint_text()
{
    if (total_glyph_count == 0) return;

    label_buffers.Bind();
    glActiveTexture(GL_TEXTURE0);
    main_font.bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6 * total_glyph_count), GL_UNSIGNED_INT, 0);
    main_font.unbind();
    label_buffers.UnBind();
}

uint32_t label_text_store::get_buffer(const label_text& lb,
    float* vertices, uint32_t& vertex_index, uint32_t* indices, uint32_t& indices_index)
{
    hb_buffer_t* hb_buf = hb_buffer_create();
    hb_buffer_add_utf8(hb_buf, lb.label.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(hb_buf);
    hb_shape(main_font.hb_font, hb_buf, nullptr, 0);

    unsigned int         glyph_count = 0;
    hb_glyph_info_t*     glyph_info  = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);
    hb_glyph_position_t* glyph_pos   = hb_buffer_get_glyph_positions(hb_buf, &glyph_count);

    const glm::vec2 loc    = lb.label_loc;
    const float     x_scale = lb.label_x_size;
    const float     y_scale = lb.label_y_size;
    float cx = loc.x;
    float cy = loc.y;

    uint32_t glyphs_written = 0;

    for (unsigned int i = 0; i < glyph_count; i++) {
        const uint32_t gid = glyph_info[i].codepoint;

        const float x_offset  = (glyph_pos[i].x_offset  / 64.0f) * x_scale;
        const float y_offset  = (glyph_pos[i].y_offset  / 64.0f) * y_scale;
        const float x_advance = (glyph_pos[i].x_advance / 64.0f) * x_scale;

        auto it = main_font.glyph_map.find(gid);
        if (it == main_font.glyph_map.end()) {
            cx += x_advance;
            continue;
        }
        const GlyphData& gd = it->second;

        const float xpos = cx + x_offset + gd.bearing.x * x_scale;
        const float ypos = cy + y_offset - (gd.size.y - gd.bearing.y) * y_scale;
        const float w    = gd.size.x * x_scale;
        const float h    = gd.size.y * y_scale;

        constexpr float margin = 0.00002f;
        const glm::vec2 uv_tl = gd.uv_top_left  + glm::vec2( margin, 0.0f);
        const glm::vec2 uv_br = gd.uv_bot_right  + glm::vec2(-margin, 0.0f);

        auto write_vertex = [&](float px, float py, float u, float v) {
            glm::vec2 rp = rotate_pt(loc, glm::vec2(px, py), lb.label_angle);
            vertices[vertex_index + 0] = rp.x;
            vertices[vertex_index + 1] = rp.y;
            vertices[vertex_index + 2] = loc.x;
            vertices[vertex_index + 3] = loc.y;
            vertices[vertex_index + 4] = u;
            vertices[vertex_index + 5] = v;
            vertices[vertex_index + 6] = lb.label_color.r;
            vertices[vertex_index + 7] = lb.label_color.g;
            vertices[vertex_index + 8] = lb.label_color.b;
            vertex_index += 9;
        };

        write_vertex(xpos,     ypos + h, uv_tl.x, uv_tl.y);  // top-left
        write_vertex(xpos,     ypos,     uv_tl.x, uv_br.y);  // bottom-left
        write_vertex(xpos + w, ypos,     uv_br.x, uv_br.y);  // bottom-right
        write_vertex(xpos + w, ypos + h, uv_br.x, uv_tl.y);  // top-right

        const uint32_t base = (indices_index / 6) * 4;
        indices[indices_index + 0] = base + 0;
        indices[indices_index + 1] = base + 1;
        indices[indices_index + 2] = base + 2;
        indices[indices_index + 3] = base + 2;
        indices[indices_index + 4] = base + 3;
        indices[indices_index + 5] = base + 0;
        indices_index += 6;

        cx += x_advance;
        glyphs_written++;
    }

    hb_buffer_destroy(hb_buf);
    return glyphs_written;
}

glm::vec2 label_text_store::rotate_pt(const glm::vec2& rotate_about, const glm::vec2& pt, float rotation_angle)
{
    const glm::vec2 translated = pt - rotate_about;
    const float radians   = (rotation_angle * static_cast<float>(M_PI)) / 180.0f;
    const float cos_theta = std::cos(radians);
    const float sin_theta = std::sin(radians);
    const glm::vec2 rotated(translated.x * cos_theta - translated.y * sin_theta,
                            translated.x * sin_theta + translated.y * cos_theta);
    return rotated + rotate_about;
}
