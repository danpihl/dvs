#ifndef NEW_QT_APPLICATION_GUI_ELEMENT_H_
#define NEW_QT_APPLICATION_GUI_ELEMENT_H_

#include <QCursor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QSize>
#include <QWidget>

#include <functional>
#include <memory>
#include <utility>

#include "color.h"
#include "gui_element_state.h"
#include "lumos/math.h"
#include "lumos/plotting/enumerations.h"
#include "lumos/plotting/fillable_uint8_array.h"
#include "project_state/project_settings.h"

using namespace lumos;

enum class CursorSquareState
{
    INSIDE,
    OUTSIDE,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
};

struct Bound2D
{
    float x_min;
    float x_max;
    float y_min;
    float y_max;

    Bound2D() = default;

    bool pointIsWithin(const QPoint& point) const
    {
        return (x_min <= point.x()) && (point.x() <= x_max) && (y_min <= point.y()) && (point.y() <= y_max);
    }
};

// Adapted from main_application/gui_element.h's ApplicationGuiElement, with
// the design simplified relative to wx where fidelity wasn't required (see
// CURRENT_STATE.md for the reasoning, agreed with the user):
//  - Element geometry is a plain fraction of its parent's size — no margin/
//    ratio adjustment for a reserved tab-strip region (wx's minimum_x_pos_/
//    minimum_y_pos_). The tab-strip now gets its own space via a real
//    QHBoxLayout in GuiWindow, so elements never need to know about it.
//    This is a deliberate break from wx's on-disk coordinate convention;
//    old .duoplot files need a one-time re-save, which was judged an
//    acceptable trade for removing a whole category of margin-math bugs.
//  - A single modifier (Qt::ControlModifier, which Qt already reports as
//    Cmd on macOS and Ctrl elsewhere) drives both the resize-cursor preview
//    and the actual click-drag resize/move, replacing wx's Mac-specific
//    split between WXK_COMMAND (preview) and WXK_CONTROL (trigger).
//
// Kept as a plain (non-QObject) mixin, exactly like the wx original, so
// concrete elements use multiple inheritance: e.g.
//   class ButtonGuiElement : public QPushButton, public GuiElement
class GuiElement
{
protected:
    std::shared_ptr<ElementSettings> element_settings_;

    std::function<void(const char key)> notify_main_window_key_pressed_;
    std::function<void(const char key)> notify_main_window_key_released_;
    std::function<void(const QPoint pos, const std::string& elem_name)> notify_parent_window_right_mouse_pressed_;
    std::function<void()> notify_main_window_about_modification_;
    std::function<void(const QPoint& pos, const QSize& size, const bool is_editing)> notify_tab_about_editing_;
    std::function<void(const Color_t, const std::string&)> push_text_to_cmdl_output_window_;

    GuiElementId id_;

    int min_x_size_;
    int max_x_size_;
    int min_y_size_;
    int max_y_size_;

    bool mouse_is_inside_;

    QPoint current_mouse_pos_;
    QPoint previous_mouse_pos_;

    QPoint mouse_pos_at_press_;

    QPoint element_pos_at_press_;
    QSize element_size_at_press_;

    bool shift_pressed_at_mouse_press_;
    bool control_pressed_at_mouse_press_;
    bool left_mouse_pressed_;

    CursorSquareState cursor_state_at_press_;

    float edit_size_margin_;

    void sendGuiData();
    virtual std::uint64_t getGuiPayloadSize() const = 0;
    virtual void fillGuiPayload(class FillableUInt8Array& output_array) const = 0;
    void fillWithBasicData(class FillableUInt8Array& output_array) const;
    std::uint64_t basicDataSize() const;

    std::pair<Bound2D, Bound2D> getBounds() const
    {
        Bound2D bnd;
        bnd.x_min = 0.0f;
        bnd.x_max = this->getSize().width();
        bnd.y_min = 0.0f;
        bnd.y_max = this->getSize().height();

        Bound2D bnd_with_margin;
        bnd_with_margin.x_min = bnd.x_min + edit_size_margin_;
        bnd_with_margin.x_max = bnd.x_max - edit_size_margin_;
        bnd_with_margin.y_min = bnd.y_min + edit_size_margin_;
        bnd_with_margin.y_max = bnd.y_max - edit_size_margin_;

        return std::make_pair(bnd, bnd_with_margin);
    }

public:
    GuiElement() = delete;
    GuiElement(const std::shared_ptr<ElementSettings>& element_settings,
               const std::function<void(const char key)>& notify_main_window_key_pressed,
               const std::function<void(const char key)>& notify_main_window_key_released,
               const std::function<void(const QPoint pos, const std::string& elem_name)>&
                   notify_parent_window_right_mouse_pressed,
               const std::function<void()>& notify_main_window_about_modification,
               const std::function<void(const QPoint& pos, const QSize& size, const bool is_editing)>&
                   notify_tab_about_editing,
               const std::function<void(const Color_t, const std::string&)>& push_text_to_cmdl_output_window);

    virtual ~GuiElement() {}

    virtual void hide() {}
    virtual void show() {}

    std::string getHandleString() const
    {
        return element_settings_->handle_string;
    }

    virtual void setHandleString(const std::string& new_name)
    {
        element_settings_->handle_string = new_name;
    }

    virtual void updateElementSettings(const std::map<std::string, std::string>& new_settings) = 0;

    std::shared_ptr<ElementSettings> getElementSettings() const
    {
        return element_settings_;
    }

    // Not pure virtual: the position/size formula itself (setElementPositionAndSize)
    // is uniform across every element type and implemented once, faithfully, in
    // gui_element.cpp. Only the actual application of a computed geometry to the
    // underlying widget (setPosition/setSize) is per-element-type.
    void updateSizeFromParent(const QSize& parent_size);

    virtual QPoint getPosition() const = 0;
    virtual QSize getSize() const = 0;
    virtual QWidget* getParentWidget() const = 0;

    virtual void setPosition(const QPoint& new_pos) = 0;
    virtual void setSize(const QSize& new_size) = 0;

    virtual void setLabel(const std::string& /*new_label*/) {}

    virtual std::shared_ptr<GuiElementState> getGuiElementState() const
    {
        return std::make_shared<GuiElementState>();
    }

    CursorSquareState getCursorSquareState(const Bound2D bound, const Bound2D bound_margin, const QPoint mouse_pos);

    void setCursorDependingOnMousePos(const QPoint& current_mouse_position);

    void adjustPaneSizeOnMouseMoved();
    virtual void setElementPositionAndSize();

    // Keyboard functions
    virtual void keyPressedElementSpecific(const char key) = 0;
    virtual void keyReleasedElementSpecific(const char key) = 0;

    void keyPressed(const char key);
    void keyReleased(const char key);

    void keyPressedCallback(QKeyEvent* evt);
    void keyReleasedCallback(QKeyEvent* evt);

    // Mouse functions
    virtual void mouseLeftPressedGuiElementSpecific(QMouseEvent* /*event*/) {}
    virtual void mouseMovedGuiElementSpecific(QMouseEvent* /*event*/) {}
    virtual void mouseLeftReleasedGuiElementSpecific(QMouseEvent* /*event*/) {}

    virtual void mouseRightPressedGuiElementSpecific(QMouseEvent* /*event*/) {}
    virtual void mouseRightLeftGuiElementSpecific(QMouseEvent* /*event*/) {}

    void mouseEnteredElement(QEvent* event);
    void mouseLeftElement(QEvent* event);

    void mouseMovedOverItem(QMouseEvent* event);

    void mouseLeftReleased(QMouseEvent* event);
    void mouseLeftPressed(QMouseEvent* event);

    void mouseRightReleased(QMouseEvent* event);
    void mouseRightPressed(QMouseEvent* event);
};

#endif  // NEW_QT_APPLICATION_GUI_ELEMENT_H_
