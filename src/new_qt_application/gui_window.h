#ifndef NEW_QT_APPLICATION_GUI_WINDOW_H_
#define NEW_QT_APPLICATION_GUI_WINDOW_H_

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "gui_element.h"
#include "project_state/project_settings.h"
#include "window_tab.h"

// Adapted from main_application/gui_window.{h,cpp}'s GuiWindow.
//
// Uses a real QHBoxLayout: a fixed-width tab-button strip plus an expanding
// `content_area_` that every WindowTab element is parented to and sized
// against. This was deliberately NOT used earlier in the port (wx computes
// element geometry as a fraction of the *full* window size minus a manual
// margin, so a physically-separate content area would have been a different,
// non-equivalent notion of "available size"). Once exact on-disk coordinate
// compatibility with old .duoplot files was no longer a requirement (see
// CURRENT_STATE.md), GuiElement's formula was simplified to a plain fraction
// of its immediate parent's size, which makes a real QLayout the natural,
// idiomatic Qt choice — Qt handles the tab-strip's reserved space for us
// instead of every element needing to know about it.
//
// The popup-menu system is ported: right-click on the window background,
// on a GUI element, or on a tab button shows the matching one of
// popup_menu_window_/_element_/_tab_ (built once in buildPopupMenus(),
// matching main_application/gui_window.cpp:134-220's three-menu structure).
// Uses direct QAction-per-item lambda connections instead of wx's
// ID+Bind() indirection — Qt has no need for the numeric-ID dance, so the
// create*/edit*/delete*/raise*/lower*/toggle* methods below are private,
// called only from those lambdas, unlike wx's public wxCommandEvent-taking
// slots. HelpPane is still out of scope per the native-chrome decision.
// Matches wx's own actual scope for menu-driven element creation: the
// "New element" submenu lists all 8 types (matching wx's items list) but
// only Plot pane/Button/Slider/Checkbox/Text label are wired up — List
// box/Editable text/Dropdown menu/Radio button group have empty handlers
// in wx itself (main_application/gui_window.cpp:755-794), so they're
// present-but-inert here too. WindowTab already has working
// createNewListBox/createNewEditableText/createDropdownMenu/
// createRadioButtonGroup methods (built for the wire-protocol path), so
// wiring these up later is trivial if ever wanted — just not something wx
// itself does.
class GuiWindow : public QMainWindow
{
    Q_OBJECT

private:
    enum class ClickSource
    {
        TAB_BUTTON,
        GUI_ELEMENT,
        THIS
    };

    QWidget* central_;
    QWidget* tab_button_panel_;
    QWidget* content_area_;
    std::vector<QPushButton*> tab_button_widgets_;
    std::vector<WindowTab*> tabs_;
    int callback_id_;
    int current_tab_num_;

    QMenu* new_element_menu_window_;
    QMenu* new_element_menu_element_;
    QMenu* new_element_menu_tab_;
    QMenu* popup_menu_window_;
    QMenu* popup_menu_element_;
    QMenu* popup_menu_tab_;

    std::function<void(const char key)> notify_main_window_key_pressed_;
    std::function<void(const char key)> notify_main_window_key_released_;
    std::function<void(const std::string&)> notify_main_window_element_deleted_;
    std::function<std::vector<std::string>(void)> get_all_element_names_;
    std::function<void(const std::string&, const std::string&)> notify_main_window_element_name_changed_;
    std::function<void(const std::string&, const std::string&)> notify_main_window_name_changed_;
    std::function<void()> notify_main_window_about_modification_;
    std::function<void(const Color_t, const std::string&)> push_text_to_cmdl_output_window_;

    std::function<void(const QPoint pos, const std::string& elem_name)> notify_parent_window_right_mouse_pressed_;

    std::string name_;
    QWidget* main_window_;

    std::string last_clicked_item_;
    std::string project_name_;
    bool project_is_saved_;
    bool laid_out_once_;

    void tabChanged(const std::string& name);
    void layoutTabButtons();
    void mouseRightPressed(const QPoint pos, const ClickSource source, const std::string& item_name);
    void addTabButton(WindowTab* tab);

    void buildPopupMenus();
    void addModeRadioGroup(QMenu* menu);
    void setMouseInteractionModeForAllTabs(const std::string& mode);
    QAction* getMenuActionByText(QMenu* menu, const std::string& text) const;
    void checkModeInAllMenus(const std::string& text);
    WindowTab* getSelectedTab() const;
    std::map<std::string, std::string> getValidNewElementHandleString(
        const std::map<std::string, std::pair<std::string, std::string>>& fields);
    std::shared_ptr<ElementSettings> findElementSettings(const std::string& handle_string) const;

    void editWindowName();
    void newTab();
    void editTabName();
    void deleteTab();

    void createNewPlotPaneCallback();
    void createNewButtonCallback();
    void createNewSliderCallback();
    void createNewCheckboxCallback();
    void createNewTextLabelCallback();

    void editElementName();
    void deleteElementAction();
    void raiseElementAction();
    void lowerElementAction();
    void toggleProjectionModeAction();

    void printGuiCode();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;

public:
    GuiWindow() = delete;
    GuiWindow(QWidget* main_window,
              const WindowSettings& window_settings,
              const std::string& project_name,
              const int callback_id,
              const bool project_is_saved,
              const std::function<void(const char key)>& notify_main_window_key_pressed,
              const std::function<void(const char key)>& notify_main_window_key_released,
              const std::function<std::vector<std::string>(void)>& get_all_element_names,
              const std::function<void(const std::string&)>& notify_main_window_element_deleted,
              const std::function<void(const std::string&, const std::string&)>& notify_main_window_element_name_changed,
              const std::function<void(const std::string&, const std::string&)>& notify_main_window_name_changed,
              const std::function<void()>& notify_main_window_about_modification,
              const std::function<void(const Color_t, const std::string&)>& push_text_to_cmdl_output_window);
    ~GuiWindow() override;

    int getCallbackId() const;

    void setName(const std::string& new_name);
    void updateLabel();
    WindowSettings getWindowSettings() const;
    std::string getName() const;
    void setIsFileSavedForLabel(const bool is_saved);
    void setProjectName(const std::string& project_name);
    void deleteAllTabs();

    GuiElement* getGuiElement(const std::string& element_handle_string) const;

    void notifyChildrenOnKeyPressed(const char key);
    void notifyChildrenOnKeyReleased(const char key);

    void createNewPlotPane();
    void createNewPlotPane(const std::string& handle_string);

    void updateAllElements();
    std::vector<GuiElement*> getGuiElements() const;
    std::vector<GuiElement*> getPlotPanes() const;
    std::vector<GuiElement*> getAllGuiElements() const;

    void screenshot(const std::string& base_path);

    std::vector<std::string> getElementNames() const;
};

#endif  // NEW_QT_APPLICATION_GUI_WINDOW_H_
