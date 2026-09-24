#include "main_window.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <chrono>
#include <iostream>

#include "filesystem.h"
#include "preferences_dialog.h"

#include "lumos/logging.h"

MainWindow::MainWindow(const std::vector<std::string>& /*cmdl_args*/)
    : QMainWindow(nullptr),
      // Preserved verbatim from the wx original (main_window.cpp:38) — a
      // hardcoded dev-machine serial device path, not a new decision. Fails
      // harmlessly if absent (SerialInterface logs and continues), matching
      // the wx build's own observed behavior.
      serial_interface_{"/dev/tty.usbmodem142102", 115200},
      configuration_agent_{new ConfigurationAgent()},
      message_router_{nullptr},
      window_manager_{nullptr},
      project_controller_{new ProjectController(this, configuration_agent_)},
      shutdown_in_progress_{false},
      new_window_button_{nullptr},
      preferences_button_{nullptr}
{
    setWindowTitle("Duoplot");
    setGeometry(30, 130, 250, 150);

    central_ = new QWidget(this);
    setCentralWidget(central_);

    serial_interface_.start();

    // Lambdas capture `this` and reach window_manager_ only at invocation
    // time, not construction time, so it's safe to build these before
    // window_manager_ is assigned a few lines down — no GuiWindow exists
    // yet to actually call any of them.
    callbacks_.key_pressed = [this](const char key) { notifyChildrenOnKeyPressed(key); };
    callbacks_.key_released = [this](const char key) { notifyChildrenOnKeyReleased(key); };
    callbacks_.get_all_element_names = [this]() -> std::vector<std::string> { return getAllElementNames(); };
    callbacks_.element_deleted = [this](const std::string& h) { elementDeleted(h); };
    callbacks_.element_name_changed = [this](const std::string& o, const std::string& n) {
        elementNameChanged(o, n);
    };
    callbacks_.name_changed = [this](const std::string& o, const std::string& n) {
        window_manager_->windowNameChanged(o, n);
    };
    callbacks_.about_modification = [this]() { project_controller_->fileModified(); };
    // Deferred: CmdlOutputWindow/TopicTextOutputWindow aren't ported, so
    // there's no on-screen sink yet. stdout is a reasonable interim sink —
    // used today only by printGuiCallbackCode() — swap this for the real
    // window once it exists.
    callbacks_.push_text_to_cmdl_output_window = [](const Color_t, const std::string& text) { std::cout << text; };

    new_window_button_ = new QPushButton("New window", central_);
    connect(new_window_button_, &QPushButton::clicked, this, [this]() { newWindow(); });

    preferences_button_ = new QPushButton("Preferences", central_);
    connect(preferences_button_, &QPushButton::clicked, this, [this]() { openPreferences(); });

    // ProjectController's constructor already stood up SaveManager (see its
    // own constructor) — WindowManager just needs the pointer.
    window_manager_ = new WindowManager(this, central_, project_controller_->getSaveManager(), callbacks_);
    project_controller_->setWindowManager(window_manager_);

    window_manager_->setupWindows(project_controller_->getSaveManager()->getCurrentProjectSettings());
    if (window_manager_->empty())
    {
        // Deliberate deviation from wx (see the class-level comment in
        // main_window.h): a completely fresh install with no saved project
        // would leave wx with zero windows too, but that makes the port
        // untestable out of the box. Fall back to one demo window here only.
        window_manager_->bootstrapDefaultProject();
    }
    layoutWindowButtons();

    // MessageRouter talks back to us purely through these callbacks — see
    // message_router.h and ARCHITECTURE_IMPROVEMENTS.md #4. Each of the
    // first four is invoked from MessageRouter's TCP thread and delegates
    // straight to WindowManager's own thread-safe accessors (each of which
    // locks WindowManager's own elements_mtx_ internally); the last two are
    // only invoked from poll() on the GUI thread.
    MessageRouterCallbacks router_callbacks;
    router_callbacks.find_plot_pane = [this](const std::string& handle) {
        return window_manager_->findPlotPane(handle);
    };
    router_callbacks.set_element_label = [this](const std::string& handle, const std::string& label) {
        window_manager_->setElementLabel(handle, label);
    };
    router_callbacks.get_all_gui_element_states = [this]() { return window_manager_->getAllGuiElementStates(); };
    router_callbacks.perform_screenshot = [this](const std::string& path) {
        window_manager_->performScreenshot(path);
    };
    router_callbacks.open_project_file = [this](const std::string& path) { openExistingFile(path); };
    router_callbacks.create_new_window_for_element = [this](const std::string& handle) {
        window_manager_->newWindowWithoutFileModification(handle);
    };

    message_router_ = new MessageRouter(router_callbacks);
    message_router_->start();

    receive_timer_ = new QTimer(this);
    connect(receive_timer_, &QTimer::timeout, this, [this]() {
        if (!shutdown_in_progress_)
        {
            handleSerialData();
            message_router_->poll();
        }
    });
    receive_timer_->start(configuration_agent_->getVisualizationPeriodMs());

    show();

    LUMOS_LOG_INFO() << "MainWindow initialized";
}

MainWindow::~MainWindow()
{
    delete message_router_;
    delete window_manager_;
    delete project_controller_;
    delete configuration_agent_;
}

void MainWindow::newProject()
{
    project_controller_->newProject();
}

void MainWindow::saveProject()
{
    project_controller_->saveProject();
}

void MainWindow::saveProjectAs(const std::string& file_path)
{
    project_controller_->saveProjectAs(file_path);
}

void MainWindow::saveProjectAs()
{
    project_controller_->saveProjectAs();
}

void MainWindow::openExistingFile(const std::string& file_path)
{
    if (project_controller_->openExistingFile(file_path))
    {
        layoutWindowButtons();
    }
}

void MainWindow::openExistingFile()
{
    project_controller_->openExistingFile();
}

void MainWindow::layoutWindowButtons()
{
    const int button_height = 30;
    int y = window_manager_->layoutButtonsFrom(0);

    if (new_window_button_ != nullptr)
    {
        new_window_button_->setGeometry(0, y, 250, button_height);
        y += button_height;
    }

    if (preferences_button_ != nullptr)
    {
        preferences_button_->setGeometry(0, y, 250, button_height);
        y += button_height;
    }

    resize(250, std::max(150, y));
}

void MainWindow::openPreferences()
{
    PreferencesDialog dialog(this, configuration_agent_->getVisualizationPeriodMs());

    if (dialog.exec() == QDialog::Accepted)
    {
        const int new_period_ms = dialog.getVisualizationPeriodMs();
        configuration_agent_->setVisualizationPeriodMs(new_period_ms);
        receive_timer_->setInterval(new_period_ms);
    }
}

void MainWindow::toggleWindowVisibility(const std::string& window_name)
{
    window_manager_->toggleWindowVisibility(window_name);
}

void MainWindow::deleteWindow(const int callback_id)
{
    window_manager_->deleteWindow(callback_id);
    layoutWindowButtons();
}

void MainWindow::printGuiCallbackCode()
{
    window_manager_->printGuiCallbackCode();
}

std::vector<std::string> MainWindow::getAllElementNames() const
{
    return window_manager_->getAllElementNames();
}

void MainWindow::newWindow()
{
    // Matches wx's own MainWindow::newWindow()
    // (main_application/main_window.cpp:704-712): suppress
    // fileModified()/about_modification() while the window is actually
    // being built (WindowManager's construction path can trigger spurious
    // modification signals internally), then fire it once, deliberately,
    // afterward — this is what actually marks the project dirty. Previously
    // missing here (see ARCHITECTURE_IMPROVEMENTS.md #9): clicking
    // "New window" created the window but never marked the project as
    // modified, so a subsequent close/open-a-different-file could silently
    // discard it with no "unsaved changes?" prompt.
    project_controller_->beginInitialization();
    window_manager_->newWindowWithoutFileModification();
    project_controller_->endInitialization();
    project_controller_->fileModified();
}

void MainWindow::newWindowWithoutFileModification()
{
    window_manager_->newWindowWithoutFileModification();
}

void MainWindow::newWindowWithoutFileModification(const std::string& element_handle_string)
{
    window_manager_->newWindowWithoutFileModification(element_handle_string);
}

void MainWindow::elementDeleted(const std::string& element_handle_string)
{
    if (!shutdown_in_progress_)
    {
        window_manager_->elementDeleted(element_handle_string);
    }
}

void MainWindow::elementNameChanged(const std::string& old_name, const std::string& new_name)
{
    window_manager_->elementNameChanged(old_name, new_name);
}

void MainWindow::notifyChildrenOnKeyPressed(const char key)
{
    window_manager_->notifyChildrenOnKeyPressed(key);
}

void MainWindow::notifyChildrenOnKeyReleased(const char key)
{
    window_manager_->notifyChildrenOnKeyReleased(key);
}

void MainWindow::destroy()
{
    shutdown_in_progress_ = true;
    close();
}

// ---------------------------------------------------------------------------
// Receive/dispatch pipeline — extracted into MessageRouter (see
// message_router.h/.cpp and ARCHITECTURE_IMPROVEMENTS.md #4). What's left
// here is handleSerialData(), which was never part of that pipeline (it's
// tied to SerialInterface, not the TCP/wire-protocol path).
// ---------------------------------------------------------------------------

void MainWindow::handleSerialData()
{
    // Deferred: SerialInterface is started and polled (matching wx), but
    // nothing consumes the extracted frames yet — the consuming pipeline
    // (GuiElementState updates, scrolling-text topics) isn't ported. See
    // main_application/main_window_serial.cpp for the full original.
}
