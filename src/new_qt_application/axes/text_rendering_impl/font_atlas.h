#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

// Per-glyph data stored in the atlas, keyed by FreeType glyph index.
struct GlyphData
{
    glm::ivec2 size;         // bitmap width, rows
    glm::ivec2 bearing;      // bitmap_left, bitmap_top
    glm::vec2  uv_top_left;
    glm::vec2  uv_bot_right;
};

class font_atlas
{
public:
    uint32_t   texture_id   = 0;
    uint32_t   atlas_width  = 0;
    uint32_t   atlas_height = 0;

    FT_Library ft_lib  = nullptr;
    FT_Face    ft_face = nullptr;
    hb_font_t* hb_font = nullptr;  // kept alive for shaping

    // key = FreeType glyph index (not Unicode codepoint)
    std::map<uint32_t, GlyphData> glyph_map;

    font_atlas() = default;
    ~font_atlas();

    // Load font file and create the FreeType face + HarfBuzz font.
    // Does NOT build the GPU texture yet — call build_atlas() after collecting glyphs.
    bool init(const std::string& font_path, uint32_t pixel_size = 128);

    // Build the GPU texture atlas for the given set of FreeType glyph indices.
    // May be called once after all text strings are known.
    bool build_atlas(const std::vector<uint32_t>& glyph_ids);

    void bind();
    void unbind();
};
