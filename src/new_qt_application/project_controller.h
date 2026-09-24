#ifndef NEW_QT_APPLICATION_PROJECT_CONTROLLER_H_
#define NEW_QT_APPLICATION_PROJECT_CONTROLLER_H_

#include <QWidget>

#include <string>

#include "project_state/configuration_agent.h"
#include "project_state/save_manager.h"

class WindowManager;

// Extracted from MainWindow — see ARCHITECTURE_IMPROVEMENTS.md #9. Owns
// SaveManager and every project-lifecycle operation wx's own MainWindow
// mixed into its window/element bookkeeping: fileModified() (the dirty-flag
// notification every element/window mutation ultimately reaches, via
// GuiCallbacks::about_modification), newProject(), saveProject(),
// saveProjectAs() (both overloads), and openExistingFile() (both
// overloads). WindowManager keeps window/element lifecycle; MainWindow
// keeps only its own widget layout (new_window_button_/preferences_button_)
// and wire-protocol routing (MessageRouter) — it talks to this class only
// through the methods below, matching the MessageRouter/WindowManager
// extractions before it.
//
// window_initialization_in_progress_ lives here, not in WindowManager: it's
// fundamentally about whether a modification should be recorded as a real,
// dirty-worthy change (this class's job), not about window/element
// bookkeeping. MainWindow::newWindow() straddles both classes — it asks
// WindowManager to actually build the window, but must bracket that call
// with beginInitialization()/endInitialization() and then call
// fileModified() itself afterward, exactly matching wx's own
// MainWindow::newWindow() (main_application/main_window.cpp:704-712) —
// see that call site's own comment for the bug this fixes.
class ProjectController
{
public:
    ProjectController() = delete;
    ProjectController(QWidget* dialog_parent, ConfigurationAgent* configuration_agent);
    ~ProjectController();

    // Set once, right after WindowManager is constructed in MainWindow's
    // constructor. Can't be a constructor parameter here: WindowManager's
    // own constructor needs getSaveManager() to already exist, so
    // ProjectController must be built (and its SaveManager stood up)
    // first — this setter closes the resulting one-step ordering gap.
    void setWindowManager(WindowManager* window_manager);

    SaveManager* getSaveManager() const;

    // Brackets a caller-driven modification (e.g. MainWindow::newWindow()
    // building a new GuiWindow via WindowManager) so any fileModified()
    // calls from *inside* that construction are suppressed — matching wx's
    // own window_initialization_in_progress_ pattern. Also used internally
    // by newProject()/openExistingFile().
    void beginInitialization();
    void endInitialization();

    void fileModified();

    void newProject();
    void saveProject();
    void saveProjectAs();
    void saveProjectAs(const std::string& file_path);
    void openExistingFile();
    // Returns true if the project was actually (re)loaded — false if
    // file_path matched the currently open file, or failed to parse.
    // MainWindow uses this to know whether its own window-button layout
    // needs to be redone.
    bool openExistingFile(const std::string& file_path);

private:
    QWidget* dialog_parent_;
    ConfigurationAgent* configuration_agent_;
    WindowManager* window_manager_;
    SaveManager* save_manager_;

    bool window_initialization_in_progress_;
};

#endif  // NEW_QT_APPLICATION_PROJECT_CONTROLLER_H_
