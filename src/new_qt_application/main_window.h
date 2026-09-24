#ifndef NEW_QT_APPLICATION_MAIN_WINDOW_H_
#define NEW_QT_APPLICATION_MAIN_WINDOW_H_

#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "gui_callbacks.h"
#include "gui_element.h"
#include "gui_window.h"
#include "message_router.h"
#include "preferences_dialog.h"
#include "project_controller.h"
#include "project_state/configuration_agent.h"
#include "serial_interface/serial_interface.h"
#include "window_manager.h"

// Faithful port of main_application/main_window.{h,cpp} and
// main_window_receive.cpp's MainWindow — except its three original
// responsibilities have since been split out into their own classes, each
// talking back to MainWindow only through a callbacks struct or a small,
// explicit method surface: TCP receive thread/wire-protocol dispatch into
// MessageRouter (message_router.h, ARCHITECTURE_IMPROVEMENTS.md #4);
// window/element lifecycle into WindowManager (window_manager.h, the other
// half of #4); and SaveManager ownership + every project-lifecycle
// operation (fileModified/newProject/saveProject/saveProjectAs/
// openExistingFile) into ProjectController (project_controller.h,
// ARCHITECTURE_IMPROVEMENTS.md #9). What's left directly in MainWindow is
// its own widget layout (new_window_button_/preferences_button_/central_),
// ConfigurationAgent (app-level preferences, not project state), and thin
// delegating methods kept for API compatibility with GuiWindow's
// popup-menu system and MessageRouterCallbacks.
//
// Per the audit finding in AUDIT_OF_PRIOR_ATTEMPT.md: wx's MainWindow is a
// real, visible small control-panel window (one button per open GuiWindow,
// to toggle its visibility, plus a "New window" button) — not the fully
// hidden router the old qt_application attempt assumed. This port keeps it
// visible, using native Qt chrome and plain QPushButtons in place of wx's
// hand-painted WindowButton/CloseButton (which are out of scope per the
// native-chrome decision).
//
// SaveManager is now wired up (see CURRENT_STATE.md), matching wx's
// setupWindows/newProject/saveProject/saveProjectAs/openExistingFile flow.
// One deliberate, clearly-marked deviation: bootstrapDefaultProject() falls
// back to a hardcoded demo window (one of each GUI element type) only when
// SaveManager has no project to load at all (fresh install, no
// last_opened_file) — wx itself would show zero windows in that case. This
// keeps `c gui basic` and friends testable without a project file on disk.
//
// GuiWindow's popup-menu system (see its own class comment) calls back into
// deleteWindow()/newWindow()/printGuiCallbackCode() here, matching wx's
// cross-class Bind()/static_cast<MainWindow*> pattern. printGuiCallbackCode
// is ported for real (main_application/main_window.cpp:294-379's text
// generation, verbatim) but its sink, push_text_to_cmdl_output_window_,
// is stdout for now — CmdlOutputWindow itself is still deferred.
//
// preferences_button_ opens PreferencesDialog (see its own header comment):
// new functionality, not a port — wx's own "Preferences" is a dead stub
// (main_window.cpp:974 just prints "Preferences!", and the tray-icon menu
// item that would reach it is commented out), so there was nothing to
// mimic. Currently exposes the one real ConfigurationAgent-backed tunable,
// visualization_period_ms, and applies a change live via
// receive_timer_->setInterval() instead of requiring a restart (wx only
// ever reads this value once, at startup).
//
// Deferred, not dropped (see CURRENT_STATE.md): the menu bar, tray icon,
// CmdlOutputWindow/TopicTextOutputWindow, the serial two-way-handshake
// machinery, and serial data parsing (handleSerialData is a documented
// no-op — SerialInterface itself still starts and is polled, matching wx,
// but nothing consumes the frames yet since the consuming pipeline —
// GuiElementState, scrolling text — isn't ported). saveProjectAs()/
// openExistingFile() (no-arg, dialog-driven overloads) have no menu to
// trigger them yet but are implemented and ready.
class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    SerialInterface serial_interface_;

    // App-level (not per-project) preferences — visualization_period_ms,
    // and the last-opened-file path ProjectController reads/writes on its
    // behalf. Kept here, not moved into ProjectController: it's also used
    // for openPreferences()/getVisualizationPeriodMs(), which have nothing
    // to do with project save/load.
    ConfigurationAgent* configuration_agent_;

    // TCP receive thread + wire-protocol dispatch is fully extracted into
    // MessageRouter — see ARCHITECTURE_IMPROVEMENTS.md #4. MainWindow talks
    // to it only through MessageRouterCallbacks (constructed once, in the
    // constructor) and message_router_->poll() (called from receive_timer_).
    MessageRouter* message_router_;

    // Window/element lifecycle is fully extracted into WindowManager (the
    // other half of ARCHITECTURE_IMPROVEMENTS.md #4) — every open
    // GuiWindow, plot_panes_/gui_elements_, and the per-window toggle
    // buttons in the control panel.
    WindowManager* window_manager_;

    // SaveManager ownership and every project-lifecycle operation
    // (fileModified/newProject/saveProject/saveProjectAs/openExistingFile)
    // is fully extracted into ProjectController — see
    // ARCHITECTURE_IMPROVEMENTS.md #9. MainWindow's own methods of the same
    // names (below) are thin delegators, kept for API compatibility with
    // GuiWindow's popup-menu system and MessageRouterCallbacks.
    ProjectController* project_controller_;

    bool shutdown_in_progress_;

    QTimer* receive_timer_;

    QPushButton* new_window_button_;
    QPushButton* preferences_button_;
    QWidget* central_;

    // Constructed once in the constructor and passed as one bundle to
    // every GuiWindow this creates (via WindowManager) — see
    // gui_callbacks.h and ARCHITECTURE_IMPROVEMENTS.md #2. right_mouse_pressed
    // and tab_about_editing are left empty here: MainWindow doesn't
    // participate in right-click dispatch or edit-mode silhouettes,
    // GuiWindow and WindowTab fill those in themselves before passing
    // their own copies further down.
    GuiCallbacks callbacks_;

    void handleSerialData();

    void layoutWindowButtons();
    void openPreferences();

public:
    explicit MainWindow(const std::vector<std::string>& cmdl_args);
    ~MainWindow() override;

    std::vector<std::string> getAllElementNames() const;

    void toggleWindowVisibility(const std::string& window_name);

    void newWindow();
    void newWindowWithoutFileModification();
    void newWindowWithoutFileModification(const std::string& element_handle_string);
    void deleteWindow(const int callback_id);
    void printGuiCallbackCode();

    void newProject();
    void saveProject();
    void saveProjectAs();
    void saveProjectAs(const std::string& file_path);
    void openExistingFile();
    void openExistingFile(const std::string& file_path);

    void elementDeleted(const std::string& element_handle_string);
    void elementNameChanged(const std::string& old_name, const std::string& new_name);

    void notifyChildrenOnKeyPressed(const char key);
    void notifyChildrenOnKeyReleased(const char key);

    void destroy();
};

#endif  // NEW_QT_APPLICATION_MAIN_WINDOW_H_
