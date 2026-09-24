#include "window_manager.h"

#include <algorithm>

#include "lumos/logging.h"
#include "lumos/plotting/enumerations.h"
#include "main_window.h"

namespace
{
// Moved verbatim from main_window.cpp — port of the free functions of the
// same name in main_application/main_window.cpp:243-292, used by
// printGuiCallbackCode().
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

WindowManager::WindowManager(MainWindow* main_window, QWidget* button_parent, SaveManager* save_manager,
                             const GuiCallbacks& callbacks)
    : main_window_{main_window},
      button_parent_{button_parent},
      save_manager_{save_manager},
      callbacks_{callbacks},
      current_window_num_{0},
      window_callback_id_{0}
{
}

void WindowManager::addWindowButton(GuiWindow* gui_window)
{
    QPushButton* btn = new QPushButton(QString::fromStdString(gui_window->getName()), button_parent_);
    connect(btn, &QPushButton::clicked, this, [this, gui_window]() { toggleWindowVisibility(gui_window->getName()); });
    window_entries_.push_back(WindowEntry{gui_window, btn});
}

int WindowManager::layoutButtonsFrom(int y) const
{
    const int button_height = 30;
    for (const auto& entry : window_entries_)
    {
        entry.button->setGeometry(0, y, 250, button_height);
        entry.button->show();
        y += button_height;
    }
    return y;
}

bool WindowManager::empty() const
{
    return window_entries_.empty();
}

void WindowManager::setupWindows(const ProjectSettings& project_settings)
{
    // Faithful port of main_application/main_window.cpp:525-546's
    // setupWindows(), minus the plot_pane_subscriptions_/serial-topic
    // bookkeeping (serial data parsing is deferred, see CURRENT_STATE.md).
    for (const WindowSettings& ws : project_settings.getWindows())
    {
        GuiWindow* window = new GuiWindow(main_window_, ws, save_manager_->getCurrentFileName(), window_callback_id_,
                                          save_manager_->isSaved(), callbacks_);

        addWindowButton(window);
        window->show();
        window_callback_id_++;
        current_window_num_++;

        {
            // plot_panes_/gui_elements_ are read from MessageRouter's
            // callbacks (invoked from its TCP thread) as well as written
            // here from the GUI thread — every touch-point needs the same
            // lock. See ARCHITECTURE_IMPROVEMENTS.md #3 and #4.
            const std::lock_guard<std::mutex> lg(elements_mtx_);
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
    }

    LUMOS_LOG_INFO() << "Created " << window_entries_.size() << " windows";
}

void WindowManager::setIsFileSavedForAllWindows(const bool file_saved)
{
    for (const auto& entry : window_entries_)
    {
        entry.window->setIsFileSavedForLabel(file_saved);
    }
}

void WindowManager::setProjectNameForAllWindows(const std::string& project_name)
{
    for (const auto& entry : window_entries_)
    {
        entry.window->setProjectName(project_name);
    }
}

ProjectSettings WindowManager::getCurrentProjectSettings() const
{
    ProjectSettings ps;
    for (const auto& entry : window_entries_)
    {
        ps.pushBackWindowSettings(entry.window->getWindowSettings());
    }
    return ps;
}

void WindowManager::bootstrapDefaultProject()
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

    GuiWindow* window = new GuiWindow(main_window_, ws, "", window_callback_id_, true, callbacks_);

    addWindowButton(window);
    window->show();
    window_callback_id_++;
    current_window_num_++;

    {
        const std::lock_guard<std::mutex> lg(elements_mtx_);
        for (GuiElement* elem : window->getAllGuiElements())
        {
            gui_elements_[elem->getHandleString()] = elem;
        }
    }

    LUMOS_LOG_INFO() << "Created " << window_entries_.size() << " windows";
}

bool WindowManager::hasWindowWithName(const std::string& window_name) const
{
    return std::find_if(window_entries_.begin(), window_entries_.end(), [&window_name](const WindowEntry& entry) {
               return entry.window->getName() == window_name;
           }) != window_entries_.end();
}

void WindowManager::toggleWindowVisibility(const std::string& window_name)
{
    for (const auto& entry : window_entries_)
    {
        if (entry.window->getName() == window_name)
        {
            if (!entry.window->isVisible())
            {
                entry.window->show();
            }
            entry.window->raise();
        }
    }
}

void WindowManager::windowNameChanged(const std::string& old_name, const std::string& new_name)
{
    for (const auto& entry : window_entries_)
    {
        if (entry.window->getName() == old_name || entry.button->text().toStdString() == old_name)
        {
            entry.button->setText(QString::fromStdString(new_name));
            break;
        }
    }
}

void WindowManager::deleteWindow(const int callback_id)
{
    const auto q = std::find_if(window_entries_.begin(), window_entries_.end(),
                                 [callback_id](const WindowEntry& entry) {
                                     return entry.window->getCallbackId() == callback_id;
                                 });

    if (q == window_entries_.end())
    {
        return;
    }

    {
        const std::lock_guard<std::mutex> lg(elements_mtx_);
        for (GuiElement* ge : q->window->getPlotPanes())
        {
            plot_panes_.erase(ge->getHandleString());
        }
        for (GuiElement* ge : q->window->getGuiElements())
        {
            gui_elements_.erase(ge->getHandleString());
        }
    }

    q->window->deleteAllTabs();
    q->window->deleteLater();
    q->button->deleteLater();
    window_entries_.erase(q);

    callbacks_.about_modification();
}

void WindowManager::printGuiCallbackCode() const
{
    // Port of main_application/main_window.cpp:294-379. Sink is
    // callbacks_.push_text_to_cmdl_output_window — stdout for now (see its
    // default lambda's comment in main_window.cpp); swap for real once
    // CmdlOutputWindow is ported.
    callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "\n");
    callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "void userFunction()\n");
    callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "{\n");

    for (const auto& entry : window_entries_)
    {
        const WindowSettings ws{entry.window->getWindowSettings()};
        callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "    /// Window: " + ws.name + "\n");

        for (const TabSettings& ts : ws.tabs)
        {
            callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "    /// Tab: " + ts.name + "\n");

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

                callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, get_function_text);
            }

            callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "\n");
        }
    }

    callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "}\n\n\n");
    callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "int main(int argc, char** argv)\n{\n");

    for (const auto& entry : window_entries_)
    {
        const WindowSettings ws{entry.window->getWindowSettings()};
        callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "    /// Window: " + ws.name + "\n");

        for (const TabSettings& ts : ws.tabs)
        {
            callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "    /// Tab: " + ts.name + "\n");

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
                callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, cb_function_text);
            }
        }
    }

    callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "    lumos::gui::startGuiReceiveThread();\n");
    callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "    // Other client code here...\n");
    callbacks_.push_text_to_cmdl_output_window(Color_t::BLACK, "}\n");
}

void WindowManager::removeAllWindows()
{
    {
        const std::lock_guard<std::mutex> lg(elements_mtx_);
        plot_panes_.clear();
        gui_elements_.clear();
    }

    for (const auto& entry : window_entries_)
    {
        entry.window->deleteAllTabs();
        entry.window->deleteLater();
        entry.button->deleteLater();
    }
    window_entries_.clear();
}

void WindowManager::resetForNewProject()
{
    removeAllWindows();
    current_window_num_ = 0;
    newWindowWithoutFileModification();
}

std::vector<std::string> WindowManager::getAllElementNames() const
{
    std::vector<std::string> names;
    for (const auto& entry : window_entries_)
    {
        const auto window_names = entry.window->getElementNames();
        names.insert(names.end(), window_names.begin(), window_names.end());
    }
    return names;
}

void WindowManager::performScreenshot(const std::string& screenshot_base_path) const
{
    const std::string final_path =
        (!screenshot_base_path.empty() && screenshot_base_path.back() == '/') ? screenshot_base_path
                                                                              : screenshot_base_path + "/";
    for (const auto& entry : window_entries_)
    {
        entry.window->screenshot(final_path);
    }
}

void WindowManager::newWindow()
{
    newWindowWithoutFileModification();
}

void WindowManager::newWindowWithoutFileModification()
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

    GuiWindow* window = new GuiWindow(main_window_, ws, "", window_callback_id_, true, callbacks_);

    addWindowButton(window);
    window->show();
    window_callback_id_++;
    current_window_num_++;

    LUMOS_LOG_INFO() << "Created new window: " << window->getName();
}

void WindowManager::newWindowWithoutFileModification(const std::string& element_handle_string)
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

    GuiWindow* window = new GuiWindow(main_window_, ws, "", window_callback_id_, true, callbacks_);

    addWindowButton(window);
    window->show();
    window_callback_id_++;
    current_window_num_++;

    window->createNewPlotPane(element_handle_string);

    {
        const std::lock_guard<std::mutex> lg(elements_mtx_);
        for (GuiElement* elem : window->getAllGuiElements())
        {
            const std::string handle = elem->getHandleString();
            gui_elements_[handle] = elem;

            if (dynamic_cast<PlotPane*>(elem) != nullptr && plot_panes_.count(handle) == 0)
            {
                plot_panes_[handle] = elem;
            }
        }
    }

    LUMOS_LOG_INFO() << "Created new window: " << window->getName();
}

void WindowManager::elementDeleted(const std::string& element_handle_string)
{
    const std::lock_guard<std::mutex> lg(elements_mtx_);
    plot_panes_.erase(element_handle_string);
    gui_elements_.erase(element_handle_string);
}

void WindowManager::elementNameChanged(const std::string& old_name, const std::string& new_name)
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

    const std::lock_guard<std::mutex> lg(elements_mtx_);
    move_key(plot_panes_);
    move_key(gui_elements_);
}

void WindowManager::notifyChildrenOnKeyPressed(const char key)
{
    for (const auto& entry : window_entries_)
    {
        entry.window->notifyChildrenOnKeyPressed(key);
    }
}

void WindowManager::notifyChildrenOnKeyReleased(const char key)
{
    for (const auto& entry : window_entries_)
    {
        entry.window->notifyChildrenOnKeyReleased(key);
    }
}

PlotPane* WindowManager::findPlotPane(const std::string& handle) const
{
    const std::lock_guard<std::mutex> lg(elements_mtx_);
    const auto it = plot_panes_.find(handle);
    return it != plot_panes_.end() ? static_cast<PlotPane*>(it->second) : nullptr;
}

void WindowManager::setElementLabel(const std::string& handle, const std::string& label) const
{
    const std::lock_guard<std::mutex> lg(elements_mtx_);
    const auto it = gui_elements_.find(handle);
    if (it != gui_elements_.end())
    {
        it->second->setLabel(label);
    }
}

std::vector<std::shared_ptr<GuiElementState>> WindowManager::getAllGuiElementStates() const
{
    std::vector<std::shared_ptr<GuiElementState>> result;
    const std::lock_guard<std::mutex> lg(elements_mtx_);
    for (const auto& ge : gui_elements_)
    {
        result.push_back(ge.second->getGuiElementState());
    }
    return result;
}
