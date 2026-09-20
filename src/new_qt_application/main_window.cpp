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
#include "lumos/plotting/internal.h"
#include "lumos/plotting/enumerations.h"
#include "plot_objects/draw_mesh/draw_mesh.h"
#include "plot_objects/fast_plot2d/fast_plot2d.h"
#include "plot_objects/fast_plot3d/fast_plot3d.h"
#include "plot_objects/im_show/im_show.h"
#include "plot_objects/line_collection2/line_collection2.h"
#include "plot_objects/line_collection3/line_collection3.h"
#include "plot_objects/plot2d/plot2d.h"
#include "plot_objects/plot3d/plot3d.h"
#include "plot_objects/plot_collection2/plot_collection2.h"
#include "plot_objects/plot_collection3/plot_collection3.h"
#include "plot_objects/scatter/scatter.h"
#include "plot_objects/scatter3/scatter3.h"
#include "plot_objects/screen_space_primitive/screen_space_primitive.h"
#include "plot_objects/scrolling_plot2d/scrolling_plot2d.h"
#include "plot_objects/stairs/stairs.h"
#include "plot_objects/stem/stem.h"
#include "plot_objects/surf/surf.h"

using namespace lumos::internal;

namespace
{
// Faithful port of the free function of the same name in
// main_application/main_window_receive.cpp:54-156.
std::shared_ptr<const ConvertedDataBase> convertPlotObjectData(const CommunicationHeader& hdr,
                                                                const ReceivedData& received_data,
                                                                const PlotObjectAttributes& attributes,
                                                                const UserSuppliedProperties& user_supplied_properties)
{
    const Function fcn = received_data.getFunction();

    switch (fcn)
    {
        case Function::STAIRS:
            return Stairs::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::PLOT2:
            return Plot2D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::PLOT3:
            return Plot3D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::SCREEN_SPACE_PRIMITIVE:
            return ScreenSpacePrimitive::convertRawData(hdr, attributes, user_supplied_properties,
                                                        received_data.payloadData());
        case Function::FAST_PLOT2:
            return FastPlot2D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::FAST_PLOT3:
            return FastPlot3D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::LINE_COLLECTION2:
            return LineCollection2D::convertRawData(hdr, attributes, user_supplied_properties,
                                                    received_data.payloadData());
        case Function::LINE_COLLECTION3:
            return LineCollection3D::convertRawData(hdr, attributes, user_supplied_properties,
                                                    received_data.payloadData());
        case Function::STEM:
            return Stem::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::SCATTER2:
            return Scatter2D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::SCATTER3:
            return Scatter3D::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::SURF:
            return Surf::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::IM_SHOW:
            return ImShow::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::PLOT_COLLECTION2:
            return PlotCollection2D::convertRawData(hdr, attributes, user_supplied_properties,
                                                    received_data.payloadData());
        case Function::PLOT_COLLECTION3:
            return PlotCollection3D::convertRawData(hdr, attributes, user_supplied_properties,
                                                    received_data.payloadData());
        case Function::DRAW_MESH_SEPARATE_VECTORS:
        case Function::DRAW_MESH:
            return DrawMesh::convertRawData(hdr, attributes, user_supplied_properties, received_data.payloadData());
        case Function::REAL_TIME_PLOT:
            return ScrollingPlot2D::convertRawData(hdr, attributes, user_supplied_properties,
                                                   received_data.payloadData());
        default:
            throw std::runtime_error("Invalid function!");
    }
}

bool isGuiRelatedFunction(const Function fcn)
{
    return fcn == Function::SET_GUI_ELEMENT_LABEL;
}

// Port of the free functions of the same name in
// main_application/main_window.cpp:243-292, used by printGuiCallbackCode().
std::string guiElementTypeToGuiHandleString(const lumos::GuiElementType tp)
{
    switch (tp)
    {
        case lumos::GuiElementType::Slider:
            return "SliderHandle";
        case lumos::GuiElementType::Button:
            return "ButtonHandle";
        case lumos::GuiElementType::Checkbox:
            return "CheckboxHandle";
        case lumos::GuiElementType::TextLabel:
            return "TextLabelHandle";
        case lumos::GuiElementType::Unknown:
            throw std::runtime_error("guiElementTypeToGuiHandleString: Unknown GuiElementType!");
        default:
            return "unknown";
    }
}

std::string getCallbackFunctionUseFromType(const lumos::GuiElementType tp)
{
    if (tp == lumos::GuiElementType::Slider)
    {
        return "        const std::int32_t min_value = gui_element_handle.getMinValue();\n"
               "        const std::int32_t max_value = gui_element_handle.getMaxValue();\n"
               "        const std::int32_t step_size = gui_element_handle.getStepSize();\n"
               "        const std::int32_t value = gui_element_handle.getValue();\n";
    }
    else if (tp == lumos::GuiElementType::Button)
    {
        return "        const bool is_pressed = gui_element_handle.getIsPressed();\n";
    }
    else if (tp == lumos::GuiElementType::Checkbox)
    {
        return "        const bool is_checked = gui_element_handle.getIsChecked();\n";
    }
    else if (tp == lumos::GuiElementType::TextLabel)
    {
        return "TextLabelHandle";
    }
    else
    {
        throw std::runtime_error("getCallbackFunctionUseFromType: Unknown GuiElementType!");
    }
}
}  // namespace

MainWindow::MainWindow(const std::vector<std::string>& /*cmdl_args*/)
    : QMainWindow(nullptr),
      data_receiver_{},
      // Preserved verbatim from the wx original (main_window.cpp:38) — a
      // hardcoded dev-machine serial device path, not a new decision. Fails
      // harmlessly if absent (SerialInterface logs and continues), matching
      // the wx build's own observed behavior.
      serial_interface_{"/dev/tty.usbmodem142102", 115200},
      configuration_agent_{new ConfigurationAgent()},
      save_manager_{nullptr},
      shutdown_in_progress_{false},
      new_window_queued_{false},
      open_project_file_queued_{false},
      new_window_button_{nullptr},
      preferences_button_{nullptr},
      current_window_num_{0},
      window_callback_id_{0},
      window_initialization_in_progress_{true}
{
    setWindowTitle("Duoplot");
    setGeometry(30, 130, 250, 150);

    central_ = new QWidget(this);
    setCentralWidget(central_);

    serial_interface_.start();

    notification_from_gui_element_key_pressed_ = [this](const char key) { notifyChildrenOnKeyPressed(key); };
    notification_from_gui_element_key_released_ = [this](const char key) { notifyChildrenOnKeyReleased(key); };
    get_all_element_names_ = [this]() -> std::vector<std::string> { return getAllElementNames(); };
    notify_main_window_element_deleted_ = [this](const std::string& h) { elementDeleted(h); };
    notify_main_window_element_name_changed_ = [this](const std::string& o, const std::string& n) {
        elementNameChanged(o, n);
    };
    notify_main_window_name_changed_ = [this](const std::string& o, const std::string& n) {
        windowNameChanged(o, n);
    };
    notify_main_window_about_modification_ = [this]() { fileModified(); };
    // Deferred: CmdlOutputWindow/TopicTextOutputWindow aren't ported, so
    // there's no on-screen sink yet. stdout is a reasonable interim sink —
    // used today only by printGuiCallbackCode() — swap this for the real
    // window once it exists.
    push_text_to_cmdl_output_window_ = [](const Color_t, const std::string& text) { std::cout << text; };

    new_window_button_ = new QPushButton("New window", central_);
    connect(new_window_button_, &QPushButton::clicked, this, [this]() { newWindow(); });

    preferences_button_ = new QPushButton("Preferences", central_);
    connect(preferences_button_, &QPushButton::clicked, this, [this]() { openPreferences(); });

    // Faithful port of the wx constructor's SaveManager setup
    // (main_window.cpp:142-150).
    if (configuration_agent_->hasKey("last_opened_file") &&
        lumos::filesystem::exists(configuration_agent_->readValue<std::string>("last_opened_file")))
    {
        save_manager_ = new SaveManager(configuration_agent_->readValue<std::string>("last_opened_file"));
    }
    else
    {
        save_manager_ = new SaveManager();
    }

    setupWindows(save_manager_->getCurrentProjectSettings());
    if (windows_.empty())
    {
        // Deliberate deviation from wx (see the class-level comment in
        // main_window.h): a completely fresh install with no saved project
        // would leave wx with zero windows too, but that makes the port
        // untestable out of the box. Fall back to one demo window here only.
        bootstrapDefaultProject();
    }
    layoutWindowButtons();

    tcp_receive_thread_ = new std::thread(&MainWindow::tcpReceiveThreadFunction, this);

    receive_timer_ = new QTimer(this);
    connect(receive_timer_, &QTimer::timeout, this, [this]() {
        if (!shutdown_in_progress_)
        {
            handleSerialData();
            receiveData();
        }
    });
    receive_timer_->start(getVisualizationPeriodMs());

    window_initialization_in_progress_ = false;

    show();

    LUMOS_LOG_INFO() << "MainWindow initialized";
}

MainWindow::~MainWindow()
{
    delete save_manager_;
    delete configuration_agent_;
}

void MainWindow::setupWindows(const ProjectSettings& project_settings)
{
    // Faithful port of main_application/main_window.cpp:525-546's
    // setupWindows(), minus the plot_pane_subscriptions_/serial-topic
    // bookkeeping (serial data parsing is deferred, see CURRENT_STATE.md).
    for (const WindowSettings& ws : project_settings.getWindows())
    {
        GuiWindow* window = new GuiWindow(this, ws, save_manager_->getCurrentFileName(), window_callback_id_,
                                          save_manager_->isSaved(), notification_from_gui_element_key_pressed_,
                                          notification_from_gui_element_key_released_, get_all_element_names_,
                                          notify_main_window_element_deleted_, notify_main_window_element_name_changed_,
                                          notify_main_window_name_changed_, notify_main_window_about_modification_,
                                          push_text_to_cmdl_output_window_);

        windows_.push_back(window);
        addWindowButton(window);
        window->show();
        window_callback_id_++;
        current_window_num_++;

        for (GuiElement* elem : window->getAllGuiElements())
        {
            const std::string handle = elem->getHandleString();
            gui_elements_[handle] = elem;

            if (dynamic_cast<PlotPane*>(elem) != nullptr)
            {
                plot_panes_[handle] = elem;
            }
        }
    }

    LUMOS_LOG_INFO() << "Created " << windows_.size() << " windows";
}

void MainWindow::setIsFileSavedForAllWindows(const bool file_saved)
{
    for (auto* w : windows_)
    {
        w->setIsFileSavedForLabel(file_saved);
    }
}

void MainWindow::fileModified()
{
    if (window_initialization_in_progress_)
    {
        return;
    }
    save_manager_->setIsModified();
    setIsFileSavedForAllWindows(false);
}

void MainWindow::newProject()
{
    window_initialization_in_progress_ = true;

    if (!save_manager_->isSaved())
    {
        const auto reply = QMessageBox::question(this, "Please confirm", "Current content has not been saved! Proceed?",
                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
        {
            window_initialization_in_progress_ = false;
            return;
        }
    }

    removeAllWindows();
    save_manager_->reset();

    current_window_num_ = 0;
    newWindowWithoutFileModification();

    window_initialization_in_progress_ = false;
}

void MainWindow::saveProject()
{
    if (!save_manager_->savePathIsSet())
    {
        saveProjectAs();
        return;
    }

    if (save_manager_->isSaved())
    {
        return;
    }

    ProjectSettings ps;
    for (const GuiWindow* w : windows_)
    {
        ps.pushBackWindowSettings(w->getWindowSettings());
    }

    configuration_agent_->writeValue("last_opened_file", save_manager_->getCurrentFilePath());

    save_manager_->save(ps);
    setIsFileSavedForAllWindows(true);
}

void MainWindow::saveProjectAs(const std::string& file_path)
{
    configuration_agent_->writeValue("last_opened_file", file_path);

    if (file_path == save_manager_->getCurrentFilePath())
    {
        saveProject();
        return;
    }

    ProjectSettings ps;
    for (const GuiWindow* w : windows_)
    {
        ps.pushBackWindowSettings(w->getWindowSettings());
    }

    save_manager_->saveToNewFile(file_path, ps);

    for (auto* w : windows_)
    {
        w->setProjectName(save_manager_->getCurrentFileName());
    }
    setIsFileSavedForAllWindows(true);
}

void MainWindow::saveProjectAs()
{
    const QString path =
        QFileDialog::getSaveFileName(this, "Choose file to save to", "", "duoplot files (*.duoplot)");
    if (path.isEmpty())
    {
        return;
    }
    saveProjectAs(path.toStdString());
}

void MainWindow::openExistingFile(const std::string& file_path)
{
    window_initialization_in_progress_ = true;

    if (save_manager_->getCurrentFilePath() == file_path)
    {
        window_initialization_in_progress_ = false;
        return;
    }

    removeAllWindows();

    configuration_agent_->writeValue("last_opened_file", file_path);
    save_manager_->openExistingFile(file_path);

    setupWindows(save_manager_->getCurrentProjectSettings());
    layoutWindowButtons();

    window_initialization_in_progress_ = false;
}

void MainWindow::openExistingFile()
{
    if (!save_manager_->isSaved())
    {
        const auto reply = QMessageBox::question(this, "Please confirm", "Current content has not been saved! Proceed?",
                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
        {
            return;
        }
    }

    const QString path = QFileDialog::getOpenFileName(this, "Choose file to open", "", "duoplot files (*.duoplot)");
    if (path.isEmpty())
    {
        return;
    }
    openExistingFile(path.toStdString());
}

void MainWindow::bootstrapDefaultProject()
{
    // Dev-only fallback: see the class-level comment in main_window.h. Only
    // reached when SaveManager has no project to load (fresh install, no
    // last_opened_file config key).
    WindowSettings ws;
    ws.name = "main";
    ws.x = 100;
    ws.y = 100;
    ws.width = 1200;
    ws.height = 800;

    TabSettings ts;
    ts.name = "tab0";

    // Dev-only bootstrap: system_test's `c gui basic` (basic_c/gui_test.c)
    // registers callbacks for these exact handle names, but in wx they only
    // ever come into existence via the "New Element" popup menu (still
    // deferred) or a loaded project file (SaveManager, also deferred).
    // Creating them here directly is the only way to test the GUI-element
    // classes end to end until one of those exists. Remove once either is
    // ported. Layout: a simple 2-row grid, not meant to look good.
    {
        auto button = std::make_shared<ButtonSettings>();
        button->handle_string = "button0";
        button->label = "Button 0";
        button->x = 0.02f;
        button->y = 0.02f;
        button->width = 0.15f;
        button->height = 0.06f;
        ts.elements.push_back(button);

        auto slider = std::make_shared<SliderSettings>();
        slider->handle_string = "slider0";
        slider->min_value = 0;
        slider->max_value = 100;
        slider->init_value = 50;
        slider->step_size = 1;
        slider->is_horizontal = true;
        slider->x = 0.20f;
        slider->y = 0.02f;
        slider->width = 0.20f;
        slider->height = 0.06f;
        ts.elements.push_back(slider);

        auto checkbox = std::make_shared<CheckboxSettings>();
        checkbox->handle_string = "checkbox0";
        checkbox->label = "Checkbox 0";
        checkbox->x = 0.45f;
        checkbox->y = 0.02f;
        checkbox->width = 0.15f;
        checkbox->height = 0.06f;
        ts.elements.push_back(checkbox);

        auto text_label = std::make_shared<TextLabelSettings>();
        text_label->handle_string = "text_label0";
        text_label->label = "Text label 0";
        text_label->x = 0.63f;
        text_label->y = 0.02f;
        text_label->width = 0.20f;
        text_label->height = 0.06f;
        ts.elements.push_back(text_label);

        auto listbox = std::make_shared<ListBoxSettings>();
        listbox->handle_string = "listbox0";
        listbox->elements = {"Item 1", "Item 2", "Item 3"};
        listbox->x = 0.02f;
        listbox->y = 0.12f;
        listbox->width = 0.15f;
        listbox->height = 0.20f;
        ts.elements.push_back(listbox);

        auto dropdown = std::make_shared<DropdownMenuSettings>();
        dropdown->handle_string = "ddm0";
        dropdown->elements = {"Option A", "Option B", "Option C"};
        dropdown->x = 0.20f;
        dropdown->y = 0.12f;
        dropdown->width = 0.20f;
        dropdown->height = 0.06f;
        ts.elements.push_back(dropdown);

        auto radio_group = std::make_shared<RadioButtonGroupSettings>();
        radio_group->handle_string = "rbg0";
        radio_group->label = "Radio group 0";
        RadioButtonSettings rb1;
        rb1.label = "Choice 1";
        RadioButtonSettings rb2;
        rb2.label = "Choice 2";
        radio_group->radio_buttons = {rb1, rb2};
        radio_group->x = 0.45f;
        radio_group->y = 0.12f;
        radio_group->width = 0.20f;
        radio_group->height = 0.15f;
        ts.elements.push_back(radio_group);

        auto editable_text = std::make_shared<EditableTextSettings>();
        editable_text->handle_string = "text_entry";
        editable_text->init_value = "";
        editable_text->x = 0.63f;
        editable_text->y = 0.12f;
        editable_text->width = 0.20f;
        editable_text->height = 0.06f;
        ts.elements.push_back(editable_text);
    }

    ws.tabs.push_back(ts);

    GuiWindow* window = new GuiWindow(this, ws, "", window_callback_id_, true,
                                      notification_from_gui_element_key_pressed_,
                                      notification_from_gui_element_key_released_, get_all_element_names_,
                                      notify_main_window_element_deleted_, notify_main_window_element_name_changed_,
                                      notify_main_window_name_changed_, notify_main_window_about_modification_,
                                      push_text_to_cmdl_output_window_);

    windows_.push_back(window);
    addWindowButton(window);
    window->show();
    window_callback_id_++;
    current_window_num_++;

    for (GuiElement* elem : window->getAllGuiElements())
    {
        gui_elements_[elem->getHandleString()] = elem;
    }

    LUMOS_LOG_INFO() << "Created " << windows_.size() << " windows";
}

void MainWindow::addWindowButton(GuiWindow* gui_window)
{
    QPushButton* btn = new QPushButton(QString::fromStdString(gui_window->getName()), central_);
    connect(btn, &QPushButton::clicked, this, [this, gui_window]() { toggleWindowVisibility(gui_window->getName()); });
    window_buttons_.push_back(btn);
    layoutWindowButtons();
}

void MainWindow::layoutWindowButtons()
{
    const int button_height = 30;
    int y = 0;

    for (auto* btn : window_buttons_)
    {
        btn->setGeometry(0, y, 250, button_height);
        btn->show();
        y += button_height;
    }

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

int MainWindow::getVisualizationPeriodMs() const
{
    int visualization_period_ms = 10;
    if (configuration_agent_->hasKey("visualization_period_ms"))
    {
        visualization_period_ms =
            std::max(1, std::min(100, configuration_agent_->readValue<int>("visualization_period_ms")));
    }
    return visualization_period_ms;
}

void MainWindow::openPreferences()
{
    PreferencesDialog dialog(this, getVisualizationPeriodMs());

    if (dialog.exec() == QDialog::Accepted)
    {
        const int new_period_ms = dialog.getVisualizationPeriodMs();
        configuration_agent_->writeValue("visualization_period_ms", new_period_ms);
        receive_timer_->setInterval(new_period_ms);
    }
}

bool MainWindow::hasWindowWithName(const std::string& window_name)
{
    return std::find_if(windows_.begin(), windows_.end(), [&window_name](const GuiWindow* w) {
               return w->getName() == window_name;
           }) != windows_.end();
}

void MainWindow::toggleWindowVisibility(const std::string& window_name)
{
    for (auto* w : windows_)
    {
        if (w->getName() == window_name)
        {
            if (!w->isVisible())
            {
                w->show();
            }
            w->raise();
        }
    }
}

void MainWindow::windowNameChanged(const std::string& old_name, const std::string& new_name)
{
    for (size_t i = 0; i < windows_.size(); i++)
    {
        if (windows_[i]->getName() == old_name || window_buttons_[i]->text().toStdString() == old_name)
        {
            window_buttons_[i]->setText(QString::fromStdString(new_name));
            break;
        }
    }
}

void MainWindow::deleteWindow(const int callback_id)
{
    const auto q = std::find_if(windows_.begin(), windows_.end(),
                                 [callback_id](const GuiWindow* w) { return w->getCallbackId() == callback_id; });

    if (q == windows_.end())
    {
        return;
    }

    const size_t idx = static_cast<size_t>(std::distance(windows_.begin(), q));

    for (GuiElement* ge : (*q)->getPlotPanes())
    {
        plot_panes_.erase(ge->getHandleString());
    }
    for (GuiElement* ge : (*q)->getGuiElements())
    {
        gui_elements_.erase(ge->getHandleString());
    }

    (*q)->deleteAllTabs();
    (*q)->deleteLater();
    windows_.erase(q);

    if (idx < window_buttons_.size())
    {
        window_buttons_[idx]->deleteLater();
        window_buttons_.erase(window_buttons_.begin() + static_cast<long>(idx));
    }

    layoutWindowButtons();
    fileModified();
}

void MainWindow::printGuiCallbackCode()
{
    // Port of main_application/main_window.cpp:294-379. Sink is
    // push_text_to_cmdl_output_window_ — stdout for now (see its default
    // lambda's comment); swap for real once CmdlOutputWindow is ported.
    push_text_to_cmdl_output_window_(Color_t::BLACK, "\n");
    push_text_to_cmdl_output_window_(Color_t::BLACK, "void userFunction()\n");
    push_text_to_cmdl_output_window_(Color_t::BLACK, "{\n");

    for (const GuiWindow* gw : windows_)
    {
        const WindowSettings ws{gw->getWindowSettings()};
        push_text_to_cmdl_output_window_(Color_t::BLACK, "    /// Window: " + ws.name + "\n");

        for (const TabSettings& ts : ws.tabs)
        {
            push_text_to_cmdl_output_window_(Color_t::BLACK, "    /// Tab: " + ts.name + "\n");

            for (const std::shared_ptr<ElementSettings>& es : ts.elements)
            {
                const lumos::GuiElementType type{es->type};
                if (type == lumos::GuiElementType::Unknown || type == lumos::GuiElementType::PlotPane)
                {
                    continue;
                }
                const std::string handle_string{es->handle_string};

                const std::string get_function_text =
                    "    const lumos::gui::" + guiElementTypeToGuiHandleString(type) + " " + handle_string +
                    " = lumos::gui::getGuiElementHandle<lumos::gui::" + guiElementTypeToGuiHandleString(type) +
                    ">(\"" + handle_string + "\");\n";

                push_text_to_cmdl_output_window_(Color_t::BLACK, get_function_text);
            }

            push_text_to_cmdl_output_window_(Color_t::BLACK, "\n");
        }
    }

    push_text_to_cmdl_output_window_(Color_t::BLACK, "}\n\n\n");
    push_text_to_cmdl_output_window_(Color_t::BLACK, "int main(int argc, char** argv)\n{\n");

    for (const GuiWindow* gw : windows_)
    {
        const WindowSettings ws{gw->getWindowSettings()};
        push_text_to_cmdl_output_window_(Color_t::BLACK, "    /// Window: " + ws.name + "\n");

        for (const TabSettings& ts : ws.tabs)
        {
            push_text_to_cmdl_output_window_(Color_t::BLACK, "    /// Tab: " + ts.name + "\n");

            for (const std::shared_ptr<ElementSettings>& es : ts.elements)
            {
                const lumos::GuiElementType type{es->type};
                if (type == lumos::GuiElementType::Unknown || type == lumos::GuiElementType::PlotPane ||
                    type == lumos::GuiElementType::TextLabel)
                {
                    continue;
                }
                const std::string handle_string{es->handle_string};

                const std::string cb_function_text = "    lumos::gui::registerGuiCallback(\"" + handle_string +
                    "\", [](const " + guiElementTypeToGuiHandleString(type) + "& gui_element_handle) -> void {\n" +
                    getCallbackFunctionUseFromType(type) + "    });\n\n";
                push_text_to_cmdl_output_window_(Color_t::BLACK, cb_function_text);
            }
        }
    }

    push_text_to_cmdl_output_window_(Color_t::BLACK, "    lumos::gui::startGuiReceiveThread();\n");
    push_text_to_cmdl_output_window_(Color_t::BLACK, "    // Other client code here...\n");
    push_text_to_cmdl_output_window_(Color_t::BLACK, "}\n");
}

void MainWindow::removeAllWindows()
{
    plot_panes_.clear();
    gui_elements_.clear();

    for (auto* w : windows_)
    {
        w->deleteAllTabs();
        w->deleteLater();
    }
    windows_.clear();

    for (auto* btn : window_buttons_)
    {
        btn->deleteLater();
    }
    window_buttons_.clear();
}

std::vector<std::string> MainWindow::getAllElementNames() const
{
    std::vector<std::string> names;
    for (auto* w : windows_)
    {
        const auto window_names = w->getElementNames();
        names.insert(names.end(), window_names.begin(), window_names.end());
    }
    return names;
}

void MainWindow::performScreenshot(const std::string& screenshot_base_path)
{
    const std::string final_path =
        (!screenshot_base_path.empty() && screenshot_base_path.back() == '/') ? screenshot_base_path
                                                                              : screenshot_base_path + "/";
    for (auto* w : windows_)
    {
        w->screenshot(final_path);
    }
}

void MainWindow::updateClientApplicationAboutGuiState() const
{
    std::vector<std::shared_ptr<GuiElementState>> gui_elements_state;
    std::uint64_t total_num_bytes = 0U;

    for (const auto& ge : gui_elements_)
    {
        gui_elements_state.push_back(ge.second->getGuiElementState());
        total_num_bytes += gui_elements_state.back()->getTotalNumBytes();
    }

    total_num_bytes += sizeof(std::uint8_t);  // Number of gui elements

    FillableUInt8Array output_array{total_num_bytes};
    output_array.fillWithStaticType(static_cast<std::uint8_t>(gui_elements_state.size()));

    for (const auto& ges : gui_elements_state)
    {
        ges->serializeToBuffer(output_array);
    }

    sendThroughTcpInterface(output_array.view(), kGuiTcpPortNum);
}

void MainWindow::newWindow()
{
    window_initialization_in_progress_ = true;
    newWindowWithoutFileModification();
    window_initialization_in_progress_ = false;
}

void MainWindow::newWindowWithoutFileModification()
{
    WindowSettings ws;
    ws.name = "Window " + std::to_string(current_window_num_);
    ws.x = 30 + current_window_num_ * 30;
    ws.y = 30 + current_window_num_ * 30;
    ws.width = 900;
    ws.height = 700;

    TabSettings ts;
    ts.name = "tab0";
    ws.tabs.push_back(ts);

    GuiWindow* window = new GuiWindow(this, ws, "", window_callback_id_, true,
                                      notification_from_gui_element_key_pressed_,
                                      notification_from_gui_element_key_released_, get_all_element_names_,
                                      notify_main_window_element_deleted_, notify_main_window_element_name_changed_,
                                      notify_main_window_name_changed_, notify_main_window_about_modification_,
                                      push_text_to_cmdl_output_window_);

    windows_.push_back(window);
    addWindowButton(window);
    window->show();
    window_callback_id_++;
    current_window_num_++;

    LUMOS_LOG_INFO() << "Created new window: " << window->getName();
}

void MainWindow::newWindowWithoutFileModification(const std::string& element_handle_string)
{
    WindowSettings ws;
    ws.name = "Window " + std::to_string(current_window_num_);
    ws.x = 30 + current_window_num_ * 30;
    ws.y = 30 + current_window_num_ * 30;
    ws.width = 900;
    ws.height = 700;

    TabSettings ts;
    ts.name = "tab0";
    ws.tabs.push_back(ts);

    GuiWindow* window = new GuiWindow(this, ws, "", window_callback_id_, true,
                                      notification_from_gui_element_key_pressed_,
                                      notification_from_gui_element_key_released_, get_all_element_names_,
                                      notify_main_window_element_deleted_, notify_main_window_element_name_changed_,
                                      notify_main_window_name_changed_, notify_main_window_about_modification_,
                                      push_text_to_cmdl_output_window_);

    windows_.push_back(window);
    addWindowButton(window);
    window->show();
    window_callback_id_++;
    current_window_num_++;

    window->createNewPlotPane(element_handle_string);

    for (GuiElement* elem : window->getAllGuiElements())
    {
        const std::string handle = elem->getHandleString();
        gui_elements_[handle] = elem;

        if (dynamic_cast<PlotPane*>(elem) != nullptr && plot_panes_.count(handle) == 0)
        {
            plot_panes_[handle] = elem;
        }
    }

    LUMOS_LOG_INFO() << "Created new window: " << window->getName();
}

void MainWindow::elementDeleted(const std::string& element_handle_string)
{
    if (!shutdown_in_progress_)
    {
        plot_panes_.erase(element_handle_string);
        gui_elements_.erase(element_handle_string);
    }
}

void MainWindow::elementNameChanged(const std::string& old_name, const std::string& new_name)
{
    auto move_key = [&](std::map<std::string, GuiElement*>& m) {
        const auto it = m.find(old_name);
        if (it != m.end())
        {
            GuiElement* ge = it->second;
            m.erase(it);
            m[new_name] = ge;
        }
    };
    move_key(plot_panes_);
    move_key(gui_elements_);
}

void MainWindow::notifyChildrenOnKeyPressed(const char key)
{
    for (auto* w : windows_)
    {
        w->notifyChildrenOnKeyPressed(key);
    }
}

void MainWindow::notifyChildrenOnKeyReleased(const char key)
{
    for (auto* w : windows_)
    {
        w->notifyChildrenOnKeyReleased(key);
    }
}

void MainWindow::destroy()
{
    shutdown_in_progress_ = true;
    close();
}

// ---------------------------------------------------------------------------
// Receive/dispatch pipeline — faithful port of main_window_receive.cpp.
// ---------------------------------------------------------------------------

void MainWindow::setActiveView(const ReceivedData& received_data)
{
    const CommunicationHeader& hdr = received_data.getCommunicationHeader();
    const std::string name = hdr.get(CommunicationHeaderObjectType::ELEMENT_NAME).as<properties::Label>().data;

    if (name.empty())
    {
        LUMOS_LOG_WARNING() << "Label string had zero length!";
        return;
    }

    current_element_name_ = name;

    if (plot_panes_.count(current_element_name_) == 0)
    {
        new_window_queued_ = true;
    }
}

void MainWindow::addActionToQueue(ReceivedData& received_data)
{
    const Function fcn = received_data.getFunction();

    if (fcn == Function::SET_CURRENT_ELEMENT)
    {
        setActiveView(received_data);
    }
    else if (fcn == Function::FLUSH_MULTIPLE_ELEMENTS)
    {
        mainWindowFlushMultipleElements(received_data);
    }
    else if (isPlotDataFunction(fcn))
    {
        const CommunicationHeader& hdr = received_data.getCommunicationHeader();
        const PlotObjectAttributes plot_object_attributes{hdr};
        const UserSuppliedProperties user_supplied_properties{hdr};

        const std::shared_ptr<const ConvertedDataBase> converted_data =
            convertPlotObjectData(hdr, received_data, plot_object_attributes, user_supplied_properties);

        queued_data_[current_element_name_].push(std::make_unique<InputData>(
            received_data, converted_data, plot_object_attributes, user_supplied_properties));
    }
    else if (fcn == Function::PROPERTIES_EXTENSION || fcn == Function::PROPERTIES_EXTENSION_MULTIPLE)
    {
        const CommunicationHeader& hdr = received_data.getCommunicationHeader();
        const PlotObjectAttributes plot_object_attributes{hdr};
        const UserSuppliedProperties user_supplied_properties{hdr};
        queued_data_[current_element_name_].push(
            std::make_unique<InputData>(received_data, plot_object_attributes, user_supplied_properties));
    }
    else
    {
        queued_data_[current_element_name_].push(std::make_unique<InputData>(received_data));
    }
}

void MainWindow::handleGuiManipulation(ReceivedData& received_data)
{
    const CommunicationHeader& hdr = received_data.getCommunicationHeader();
    const std::string handle_string = hdr.get(CommunicationHeaderObjectType::HANDLE_STRING).as<properties::Label>().data;
    const std::string label = hdr.get(CommunicationHeaderObjectType::LABEL).as<properties::Label>().data;

    const auto it = gui_elements_.find(handle_string);
    if (it != gui_elements_.end())
    {
        it->second->setLabel(label);
    }
}

void MainWindow::manageReceivedData(ReceivedData& received_data)
{
    const Function fcn = received_data.getFunction();

    const std::lock_guard<std::mutex> lg(receive_mtx_);

    if (fcn == Function::OPEN_PROJECT_FILE)
    {
        const CommunicationHeader& hdr = received_data.getCommunicationHeader();
        queued_project_file_name_ = hdr.get(CommunicationHeaderObjectType::PROJECT_FILE_NAME).as<properties::Label>().data;
        open_project_file_queued_ = true;
    }
    else if (fcn == Function::SCREENSHOT)
    {
        const CommunicationHeader& hdr = received_data.getCommunicationHeader();
        const std::string path = hdr.get(CommunicationHeaderObjectType::SCREENSHOT_BASE_PATH).as<properties::Label>().data;
        performScreenshot(path);
    }
    else if (fcn == Function::QUERY_FOR_SYNC_OF_GUI_DATA)
    {
        updateClientApplicationAboutGuiState();
    }
    else if (isGuiRelatedFunction(fcn))
    {
        handleGuiManipulation(received_data);
    }
    else
    {
        addActionToQueue(received_data);
    }
}

void MainWindow::tcpReceiveThreadFunction()
{
    while (true)
    {
        ReceivedData received_data = data_receiver_.receiveAndGetDataFromTcp();
        if (received_data.rawData() == nullptr)
        {
            continue;
        }

        manageReceivedData(received_data);
    }
}

void MainWindow::mainWindowFlushMultipleElements(const ReceivedData& received_data)
{
    const CommunicationHeader& hdr = received_data.getCommunicationHeader();
    const uint8_t num_names = hdr.get(CommunicationHeaderObjectType::NUM_NAMES).as<uint8_t>();
    const VectorConstView<uint8_t> name_lengths{received_data.payloadData(), static_cast<size_t>(num_names)};

    std::vector<std::string> names;
    size_t idx = num_names;

    for (size_t k = 0; k < num_names; k++)
    {
        names.emplace_back();
        std::string& current_elem = names.back();
        const uint8_t current_element_length = name_lengths(k);
        for (size_t i = 0; i < current_element_length; i++)
        {
            current_elem += received_data.payloadData()[idx];
            idx++;
        }
    }

    CommunicationHeader faked_hdr{Function::FLUSH_ELEMENT};
    const uint64_t num_bytes_hdr = faked_hdr.numBytes();
    const uint64_t num_bytes = num_bytes_hdr + 1 + 2 * sizeof(uint64_t);

    FillableUInt8Array fillable_array{num_bytes};
    fillable_array.fillWithStaticType(isBigEndian());
    fillable_array.fillWithStaticType(kMagicNumber);
    fillable_array.fillWithStaticType(num_bytes);
    faked_hdr.fillBufferWithData(fillable_array);

    const UInt8ArrayView array_view{fillable_array.data(), fillable_array.size()};

    for (const auto& name : names)
    {
        ReceivedData fake_received_data{array_view.size()};
        std::memcpy(fake_received_data.rawData(), array_view.data(), array_view.size());
        fake_received_data.parseHeader();

        queued_data_[name].push(std::make_unique<InputData>(fake_received_data));
    }
}

void MainWindow::receiveData()
{
    {
        const std::lock_guard<std::mutex> lg(receive_mtx_);

        if (open_project_file_queued_)
        {
            open_project_file_queued_ = false;
            openExistingFile(queued_project_file_name_);
        }

        if (new_window_queued_)
        {
            new_window_queued_ = false;
            newWindowWithoutFileModification(current_element_name_);
        }

        for (auto& qa : queued_data_)
        {
            if (!qa.second.empty())
            {
                const std::string element_handle_string = qa.first;
                if (plot_panes_.count(element_handle_string) > 0)
                {
                    static_cast<PlotPane*>(plot_panes_[element_handle_string])->pushQueue(qa.second);
                }
            }
        }
    }
}

void MainWindow::handleSerialData()
{
    // Deferred: SerialInterface is started and polled (matching wx), but
    // nothing consumes the extracted frames yet — the consuming pipeline
    // (GuiElementState updates, scrolling-text topics) isn't ported. See
    // main_application/main_window_serial.cpp for the full original.
}
