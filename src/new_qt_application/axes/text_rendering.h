#ifndef MAIN_APPLICATION_AXES_TEXT_RENDERING_H_
#define MAIN_APPLICATION_AXES_TEXT_RENDERING_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "lumos/math.h"

// Forward declarations to avoid pulling GL headers into this public header.
class label_text_store;

// Holds per-OpenGL-context state shared among all TextRenderer instances
// that live in the same GL context (i.e. within one PlotPane).
// Copying a TextRenderer shares the same context; each new PlotPane should
// call init() on its own TextRenderer to populate fresh GL resources.
struct TextRenderContext {
    label_text_store* store{nullptr};
    unsigned int      shader_id{0};
    bool              initialized{false};
};

class TextRenderer
{
public:
    TextRenderer();
    TextRenderer(const TextRenderer& other);
    TextRenderer& operator=(const TextRenderer& other);
    ~TextRenderer() = default;

    // Initialize GL resources (shader + font atlas) in the current GL context.
    // Must be called once per pane after the GL context is made current.
    // Copies of this TextRenderer share the same context and will see the
    // initialized resources automatically.
    bool init();

    void setColor(float r, float g, float b);

    void renderTextFromCenter(const std::string_view& text,
                              float x,
                              float y,
                              float scale,
                              float axes_width,
                              float axes_height);
    void renderTextFromRightCenter(const std::string_view& text,
                                   float x,
                                   float y,
                                   float scale,
                                   float axes_width,
                                   float axes_height);
    void renderTextFromLeftCenter(const std::string_view& text,
                                  float x,
                                  float y,
                                  float scale,
                                  float axes_width,
                                  float axes_height);

    // Render all accumulated labels, then clear the batch.
    void flush();

    lumos::Vec2f calculateStringSize(const std::string_view& text,
                                     float scale,
                                     float axes_width,
                                     float axes_height) const;

    // The shared context object — public so free functions that need direct
    // access (e.g. calculateStringSize callers outside this class) can use it,
    // but prefer the member function above.
    std::shared_ptr<TextRenderContext> ctx_;

private:
    struct Label {
        std::string text;
        glm::vec2   loc;
        glm::vec3   color;
        float       x_scale;
        float       y_scale;
    };

    std::vector<Label> pending_labels_;
    glm::vec3          current_color_{0.0f, 0.0f, 0.0f};
};

#endif  // MAIN_APPLICATION_AXES_TEXT_RENDERING_H_
