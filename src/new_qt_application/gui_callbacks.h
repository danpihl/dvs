#ifndef NEW_QT_APPLICATION_GUI_CALLBACKS_H_
#define NEW_QT_APPLICATION_GUI_CALLBACKS_H_

#include <QPoint>
#include <QSize>

#include <functional>
#include <string>
#include <vector>

#include "color.h"

// Bundles the callbacks that thread from MainWindow down through GuiWindow
// and WindowTab to every GuiElement/PlotPane, replacing what used to be
// 6-9 separate std::function constructor parameters (and matching member
// variables) at each of those four layers — see ARCHITECTURE_IMPROVEMENTS.md
// #2. Not every layer uses every field:
//   - MainWindow constructs key_pressed/key_released/get_all_element_names/
//     element_deleted/element_name_changed/name_changed/about_modification/
//     push_text_to_cmdl_output_window, and leaves right_mouse_pressed/
//     tab_about_editing empty.
//   - GuiWindow fills in right_mouse_pressed (wrapping its own
//     mouseRightPressed) before passing its copy down to WindowTab.
//   - WindowTab fills in tab_about_editing (tied to its own
//     editing_silhouette_) before passing its copy down to GuiElement/
//     PlotPane.
// Each layer receives one `const GuiCallbacks&`, stores its own copy as
// `callbacks_`, optionally overwrites the field(s) it owns, and passes the
// (possibly augmented) copy to whatever it constructs beneath it.
struct GuiCallbacks
{
    std::function<void(const char key)> key_pressed;
    std::function<void(const char key)> key_released;
    std::function<void(const QPoint pos, const std::string& elem_name)> right_mouse_pressed;
    std::function<void(const QPoint& pos, const QSize& size, const bool is_editing)> tab_about_editing;
    std::function<void(const std::string&)> element_deleted;
    std::function<std::vector<std::string>(void)> get_all_element_names;
    std::function<void(const std::string&, const std::string&)> element_name_changed;
    std::function<void(const std::string&, const std::string&)> name_changed;
    std::function<void()> about_modification;
    std::function<void(const Color_t, const std::string&)> push_text_to_cmdl_output_window;
};

#endif  // NEW_QT_APPLICATION_GUI_CALLBACKS_H_
