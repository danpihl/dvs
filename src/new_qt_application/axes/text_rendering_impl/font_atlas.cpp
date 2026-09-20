#include "font_atlas.h"
#include <algorithm>

#include "opengl_low_level/opengl_header.h"

font_atlas::~font_atlas()
{
    if (hb_font)    { hb_font_destroy(hb_font);  hb_font  = nullptr; }
    if (ft_face)    { FT_Done_Face(ft_face);      ft_face  = nullptr; }
    if (ft_lib)     { FT_Done_FreeType(ft_lib);   ft_lib   = nullptr; }
    if (texture_id) { glDeleteTextures(1, &texture_id); texture_id = 0; }
}

bool font_atlas::init(const std::string& font_path, uint32_t pixel_size)
{
    if (FT_Init_FreeType(&ft_lib)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library\n";
        return false;
    }
    if (FT_New_Face(ft_lib, font_path.c_str(), 0, &ft_face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font: " << font_path << "\n";
        return false;
    }
    FT_Set_Pixel_Sizes(ft_face, 0, pixel_size);

    // Create the HarfBuzz font — kept alive so callers can shape text before
    // the atlas is built.
    hb_font = hb_ft_font_create(ft_face, nullptr);
    return true;
}

bool font_atlas::build_atlas(const std::vector<uint32_t>& glyph_ids)
{
    if (glyph_ids.empty()) return true;

    // Rebuild: destroy any previous texture.
    if (texture_id) {
        glDeleteTextures(1, &texture_id);
        texture_id = 0;
    }
    glyph_map.clear();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // First pass: measure total atlas width and maximum height.
    int total_width = 0;
    int max_height  = 0;
    for (uint32_t gid : glyph_ids) {
        if (FT_Load_Glyph(ft_face, gid, FT_LOAD_RENDER)) continue;
        total_width += static_cast<int>(ft_face->glyph->bitmap.width);
        max_height   = std::max(max_height, static_cast<int>(ft_face->glyph->bitmap.rows));
    }
    atlas_width  = static_cast<uint32_t>(total_width);
    atlas_height = static_cast<uint32_t>(max_height);

    if (atlas_width == 0 || atlas_height == 0) return false;

    // Allocate the atlas texture.
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                 static_cast<GLsizei>(atlas_width),
                 static_cast<GLsizei>(atlas_height),
                 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Second pass: upload each glyph bitmap and record UV coordinates.
    int x = 0;
    for (uint32_t gid : glyph_ids) {
        if (FT_Load_Glyph(ft_face, gid, FT_LOAD_RENDER)) continue;

        const auto& bm = ft_face->glyph->bitmap;
        if (bm.width > 0 && bm.rows > 0) {
            glPixelStorei(GL_UNPACK_ROW_LENGTH, bm.pitch);
            glTexSubImage2D(GL_TEXTURE_2D, 0,
                            x, 0,
                            static_cast<GLsizei>(bm.width),
                            static_cast<GLsizei>(bm.rows),
                            GL_RED, GL_UNSIGNED_BYTE, bm.buffer);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        }

        GlyphData gd;
        gd.size    = glm::ivec2(bm.width, bm.rows);
        gd.bearing = glm::ivec2(ft_face->glyph->bitmap_left, ft_face->glyph->bitmap_top);
        gd.uv_top_left  = glm::vec2(static_cast<float>(x) / atlas_width, 0.0f);
        gd.uv_bot_right = glm::vec2(static_cast<float>(x + bm.width) / atlas_width,
                                    static_cast<float>(bm.rows)       / atlas_height);
        glyph_map[gid] = gd;

        x += static_cast<int>(bm.width);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void font_atlas::bind()   { glBindTexture(GL_TEXTURE_2D, texture_id); }
void font_atlas::unbind() { glBindTexture(GL_TEXTURE_2D, 0); }
