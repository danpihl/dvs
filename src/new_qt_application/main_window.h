#ifndef NEW_QT_APPLICATION_MAIN_WINDOW_H_
#define NEW_QT_APPLICATION_MAIN_WINDOW_H_

#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "communication/data_receiver.h"
#include "communication/received_data.h"
#include "gui_element.h"
#include "gui_window.h"
#include "input_data.h"
#include "preferences_dialog.h"
#include "project_state/configuration_agent.h"
#include "project_state/project_settings.h"
#include "project_state/save_manager.h"
#include "serial_interface/serial_interface.h"

// Faithful port of main_application/main_window.{h,cpp} and
// main_window_receive.cpp's MainWindow.
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
    DataReceiver data_receiver_;
    SerialInterface serial_interface_;
    ConfigurationAgent* configuration_agent_;
    SaveManager* save_manager_;
    std::mutex receive_mtx_;

    std::thread* tcp_receive_thread_;
    std::map<std::string, GuiElement*> plot_panes_;
    std::map<std::string, GuiElement*> gui_elements_;

    std::map<std::string, std::queue<std::unique_ptr<InputData>>> queued_data_;

    bool shutdown_in_progress_;

    std::atomic<bool> new_window_queued_;
    std::string current_element_name_;

    std::atomic<bool> open_project_file_queued_;
    std::string queued_project_file_name_;

    QTimer* receive_timer_;

    std::vector<GuiWindow*> windows_;
    std::vector<QPushButton*> window_buttons_;
    QPushButton* new_window_button_;
    QPushButton* preferences_button_;
    QWidget* central_;
    int current_window_num_;
    int window_callback_id_;

    bool window_initialization_in_progress_;

    std::function<void(const char key)> notification_from_gui_element_key_pressed_;
    std::function<void(const char key)> notification_from_gui_element_key_released_;
    std::function<std::vector<std::string>(void)> get_all_element_names_;
    std::function<void(const std::string&)> notify_main_window_element_deleted_;
    std::function<void(const std::string&, const std::string&)> notify_main_window_element_name_changed_;
    std::function<void(const std::string&, const std::string&)> notify_main_window_name_changed_;
    std::function<void()> notify_main_window_about_modification_;
    std::function<void(const Color_t, const std::string&)> push_text_to_cmdl_output_window_;

    void setActiveView(const ReceivedData& received_data);
    void receiveData();
    void handleSerialData();
    void addActionToQueue(ReceivedData& received_data);
    void handleGuiManipulation(ReceivedData& received_data);
    void manageReceivedData(ReceivedData& received_data);
    void mainWindowFlushMultipleElements(const ReceivedData& received_data);
    void tcpReceiveThreadFunction();

    bool hasWindowWithName(const std::string& window_name);
    void windowNameChanged(const std::string& old_name, const std::string& new_name);

    void performScreenshot(const std::string& screenshot_base_path);
    void updateClientApplicationAboutGuiState() const;
    void removeAllWindows();
    void bootstrapDefaultProject();
    void setupWindows(const ProjectSettings& project_settings);
    void setIsFileSavedForAllWindows(const bool file_saved);
    void fileModified();
    void layoutWindowButtons();
    void openPreferences();
    int getVisualizationPeriodMs() const;

    void addWindowButton(GuiWindow* gui_window);

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
