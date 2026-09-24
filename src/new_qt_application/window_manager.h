#ifndef NEW_QT_APPLICATION_WINDOW_MANAGER_H_
#define NEW_QT_APPLICATION_WINDOW_MANAGER_H_

#include <QObject>
#include <QPushButton>
#include <QWidget>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "gui_callbacks.h"
#include "gui_element.h"
#include "gui_window.h"
#include "project_state/project_settings.h"
#include "project_state/save_manager.h"

class MainWindow;

// Extracted from MainWindow — the window/element-lifecycle half of
// ARCHITECTURE_IMPROVEMENTS.md #4 (the other half, TCP receive/dispatch,
// is MessageRouter). Owns every open GuiWindow, the plot_panes_/
// gui_elements_ lookup maps, and the left-hand per-window toggle buttons
// in MainWindow's control panel. Project save/load (SaveManager,
// newProject/saveProject/openExistingFile) and the control panel's own
// "New window"/"Preferences" buttons stay in MainWindow — this class only
// reaches SaveManager read-only (for setupWindows' project-name/is-saved
// fields) via a pointer passed in at construction, never owns or deletes
// it.
//
// GuiWindow's popup-menu system needs a real `MainWindow*` (not a
// `WindowManager*`) for its static_cast<MainWindow*>(main_window_) calls
// (deleteWindow/newWindow/printGuiCallbackCode all being called back on
// the actual MainWindow — see gui_window.h's class comment), so every
// GuiWindow this class constructs is still parented to the `MainWindow*`
// passed in here, not to `this`.
class WindowManager : public QObject
{
    Q_OBJECT

public:
    WindowManager() = delete;
    WindowManager(MainWindow* main_window, QWidget* button_parent, SaveManager* save_manager,
                  const GuiCallbacks& callbacks);

    void newWindow();
    void newWindowWithoutFileModification();
    void newWindowWithoutFileModification(const std::string& element_handle_string);
    void deleteWindow(const int callback_id);
    void toggleWindowVisibility(const std::string& window_name);
    void printGuiCallbackCode() const;

    void removeAllWindows();
    void setupWindows(const ProjectSettings& project_settings);
    void bootstrapDefaultProject();
    // Bundles removeAllWindows()/window-counter reset/
    // newWindowWithoutFileModification() for MainWindow::newProject() —
    // avoids exposing the window counter as its own public setter for a
    // single call site.
    void resetForNewProject();

    void elementDeleted(const std::string& element_handle_string);
    void elementNameChanged(const std::string& old_name, const std::string& new_name);
    void windowNameChanged(const std::string& old_name, const std::string& new_name);
    void setIsFileSavedForAllWindows(const bool file_saved);
    void setProjectNameForAllWindows(const std::string& project_name);

    void notifyChildrenOnKeyPressed(const char key);
    void notifyChildrenOnKeyReleased(const char key);

    bool hasWindowWithName(const std::string& window_name) const;
    std::vector<std::string> getAllElementNames() const;
    ProjectSettings getCurrentProjectSettings() const;
    bool empty() const;

    // Positions this class's own per-window toggle buttons starting at
    // vertical offset `y` (in MainWindow's central_ widget) and returns the
    // Y offset immediately below the last one — MainWindow continues
    // laying out its own new_window_button_/preferences_button_ from
    // there. WindowManager never calls resize() on MainWindow itself: it
    // isn't a QWidget, and the overall control-panel size is MainWindow's
    // own concern.
    int layoutButtonsFrom(int y) const;

    // Thread-safe: invoked from MessageRouterCallbacks lambdas running on
    // MessageRouter's TCP thread as well as from the GUI thread here, all
    // guarded by elements_mtx_. See ARCHITECTURE_IMPROVEMENTS.md #3.
    PlotPane* findPlotPane(const std::string& handle) const;
    void setElementLabel(const std::string& handle, const std::string& label) const;
    std::vector<std::shared_ptr<GuiElementState>> getAllGuiElementStates() const;
    void performScreenshot(const std::string& screenshot_base_path) const;

private:
    // Paired instead of two parallel vectors kept in sync by convention —
    // see ARCHITECTURE_IMPROVEMENTS.md #1 (same reasoning as GuiWindow's
    // TabEntry).
    struct WindowEntry
    {
        GuiWindow* window;
        QPushButton* button;
    };

    void addWindowButton(GuiWindow* gui_window);

    MainWindow* main_window_;
    QWidget* button_parent_;
    SaveManager* save_manager_;
    GuiCallbacks callbacks_;

    std::vector<WindowEntry> window_entries_;
    int current_window_num_;
    int window_callback_id_;

    // Guards plot_panes_/gui_elements_ only — window_entries_ is touched
    // exclusively from the GUI thread (every method here except the four
    // "Thread-safe" ones above runs there), so it needs no lock of its
    // own.
    mutable std::mutex elements_mtx_;
    std::map<std::string, GuiElement*> plot_panes_;
    std::map<std::string, GuiElement*> gui_elements_;
};

#endif  // NEW_QT_APPLICATION_WINDOW_MANAGER_H_
