#include "window_tab.h"

#include <algorithm>

#include "gui_elements.h"
#include "lumos/logging.h"

// ---------------------------------------------------------------------------
// ZOrderQueue
// ---------------------------------------------------------------------------

bool ZOrderQueue::elementExistsInQueue(const std::string& element_handle_string) const
{
    return std::find(elements_.begin(), elements_.end(), element_handle_string) != elements_.end();
}

int ZOrderQueue::getOrderOfElement(const std::string& element_handle_string) const
{
    if (elementExistsInQueue(element_handle_string))
    {
        const auto q = std::find(elements_.begin(), elements_.end(), element_handle_string);
        return static_cast<int>(q - elements_.begin());
    }
    return -1;
}

void ZOrderQueue::raise(const std::string& element_handle_string)
{
    eraseElement(element_handle_string);
    elements_.push_back(element_handle_string);
}

void ZOrderQueue::lower(const std::string& element_handle_string)
{
    eraseElement(element_handle_string);
    elements_.insert(elements_.begin(), element_handle_string);
}

void ZOrderQueue::eraseElement(const std::string& element_handle_string)
{
    const auto q = std::find(elements_.begin(), elements_.end(), element_handle_string);
    if (q != elements_.end())
    {
        elements_.erase(q);
    }
}

// ---------------------------------------------------------------------------
// getPosAndSizeInPixelCoords — placeholder construction geometry only; every
// caller immediately follows up with setMinXPos + updateSizeFromParent
// (GuiElement::setElementPositionAndSize), which applies the real formula.
// ---------------------------------------------------------------------------

std::pair<QPoint, QSize> getPosAndSizeInPixelCoords(const QSize& current_window_size,
                                                    const ElementSettings* const element_settings)
{
    const int x = static_cast<int>(element_settings->x * current_window_size.width());
    const int y = static_cast<int>(element_settings->y * current_window_size.height());
    const int w = static_cast<int>(element_settings->width * current_window_size.width());
    const int h = static_cast<int>(element_settings->height * current_window_size.height());

    return std::make_pair(QPoint(x, y), QSize(w, h));
}

// ---------------------------------------------------------------------------
// WindowTab
// ---------------------------------------------------------------------------

WindowTab::WindowTab(QWidget* parent_window, const TabSettings& tab_settings, const GuiCallbacks& callbacks)
    : name_{tab_settings.name},
      parent_window_{parent_window},
      callbacks_{callbacks},
      current_element_idx_{0}
{
    background_color_ = tab_settings.background_color;
    button_normal_color_ = tab_settings.button_normal_color;
    button_clicked_color_ = tab_settings.button_clicked_color;
    button_selected_color_ = tab_settings.button_selected_color;
    button_text_color_ = tab_settings.button_text_color;

    editing_silhouette_ = new EditingSilhouette(parent_window_, QPoint{0, 0}, QSize{100, 100});

    // Overwrites the field received from GuiWindow — this tab is the sole
    // owner of editing_silhouette_, so it must supply its own version of
    // this one callback before passing callbacks_ down to any element it
    // creates. See gui_callbacks.h.
    callbacks_.tab_about_editing = [this](const QPoint& pos, const QSize& size, const bool is_editing) -> void {
        if (is_editing)
        {
            editing_silhouette_->setPosAndSize(pos, size);
            editing_silhouette_->show();
        }
        else
        {
            editing_silhouette_->hide();
        }
    };

    for (const std::shared_ptr<ElementSettings>& elem_settings : tab_settings.elements)
    {
        switch (elem_settings->type)
        {
            case lumos::GuiElementType::PlotPane:
                createNewPlotPane(elem_settings);
                break;
            case lumos::GuiElementType::Button:
                createNewButton(elem_settings);
                break;
            case lumos::GuiElementType::Slider:
                createNewSlider(elem_settings);
                break;
            case lumos::GuiElementType::Checkbox:
                createNewCheckbox(elem_settings);
                break;
            case lumos::GuiElementType::TextLabel:
                createNewTextLabel(elem_settings);
                break;
            case lumos::GuiElementType::ListBox:
                createNewListBox(elem_settings);
                break;
            case lumos::GuiElementType::EditableText:
                createNewEditableText(elem_settings);
                break;
            case lumos::GuiElementType::DropdownMenu:
                createDropdownMenu(elem_settings);
                break;
            case lumos::GuiElementType::RadioButtonGroup:
                createRadioButtonGroup(elem_settings);
                break;
            case lumos::GuiElementType::ScrollingText:
                createScrollingText(elem_settings);
                break;
            default:
                LUMOS_LOG_WARNING() << "WindowTab: unknown GuiElementType in tab settings, skipping";
        }
    }

    initializeZOrder(tab_settings);

    LUMOS_LOG_INFO() << "WindowTab '" << name_ << "' created with " << plot_panes_.size() << " plot panes";
}

void WindowTab::initializeZOrder(const TabSettings& tab_settings)
{
    struct ZOrderPair
    {
        int order;
        std::string handle_string;
    };

    std::vector<ZOrderPair> z_order;

    for (const auto& elem : tab_settings.elements)
    {
        if (elem->z_order != -1)
        {
            z_order.push_back({elem->z_order, elem->handle_string});
        }
    }

    std::sort(z_order.begin(), z_order.end(),
              [](const ZOrderPair& p0, const ZOrderPair& p1) -> bool { return p0.order < p1.order; });

    for (const auto& zp : z_order)
    {
        raiseElement(zp.handle_string);
    }
}

WindowTab::~WindowTab()
{
    for (const auto& pp : plot_panes_)
    {
        callbacks_.element_deleted(pp->getHandleString());
        delete pp;
    }

    for (const auto& elem : gui_elements_)
    {
        callbacks_.element_deleted(elem->getHandleString());
        delete elem;
    }
}

std::vector<GuiElement*> WindowTab::getGuiElements() const
{
    return gui_elements_;
}

std::vector<GuiElement*> WindowTab::getPlotPanes() const
{
    std::vector<GuiElement*> elems;
    for (const auto& pp : plot_panes_)
    {
        elems.push_back(pp);
    }
    return elems;
}

std::vector<GuiElement*> WindowTab::getAllGuiElements() const
{
    std::vector<GuiElement*> elems = gui_elements_;
    for (const auto& pp : plot_panes_)
    {
        elems.push_back(pp);
    }
    return elems;
}

void WindowTab::updateAllElements()
{
    for (auto& pp : plot_panes_)
    {
        pp->update();
    }
}

void WindowTab::createNewPlotPane()
{
    // Matches wx's menu-triggered "New Plot Pane" default geometry exactly
    // (main_application/gui_tab.cpp:421-444) — a small pane in the top-left,
    // not full-window.
    std::shared_ptr<PlotPaneSettings> pp_settings = std::make_shared<PlotPaneSettings>();
    pp_settings->x = 0.1;
    pp_settings->y = 0.0;
    pp_settings->width = 0.4;
    pp_settings->height = 0.4;
    pp_settings->handle_string = "element-" + std::to_string(current_element_idx_);
    pp_settings->title = pp_settings->handle_string;

    PlotPane* const pp = new PlotPane(parent_window_, pp_settings, background_color_, callbacks_);
    pp->updateSizeFromParent(parent_window_->size());
    pp->show();
    plot_panes_.push_back(pp);
    current_element_idx_++;
}

void WindowTab::createNewPlotPane(const std::string& element_handle_string)
{
    // Matches wx exactly (main_application/gui_tab.cpp:446-453): only the
    // handle/title are set here — x/y/width/height are left at
    // PlotPaneSettings's own defaults (0, 0, 0.4, 0.4), not an arbitrary
    // pixel-looking placeholder. This is the fix for the old qt_application's
    // "Bug B" (hardcoded 600x400 written into fraction-typed fields — see
    // AUDIT_OF_PRIOR_ATTEMPT.md).
    std::shared_ptr<PlotPaneSettings> pp_settings = std::make_shared<PlotPaneSettings>();
    pp_settings->handle_string = element_handle_string;
    pp_settings->title = element_handle_string;

    createNewPlotPane(pp_settings);
}

void WindowTab::createNewPlotPane(const std::shared_ptr<ElementSettings>& element_settings)
{
    PlotPane* const pp = new PlotPane(parent_window_, element_settings, background_color_, callbacks_);
    // The fix for the old qt_application's "Bug A": every element, on every
    // creation path, must have its real geometry applied immediately via
    // setElementPositionAndSize (reached through updateSizeFromParent) —
    // never a raw setGeometry() using the stored fraction as if it were
    // already pixels.
    pp->updateSizeFromParent(parent_window_->size());
    pp->show();
    plot_panes_.push_back(pp);
    current_element_idx_++;

    LUMOS_LOG_INFO() << "Created plot pane: " << element_settings->handle_string;
}

void WindowTab::createNewButton(const std::shared_ptr<ElementSettings>& elem_settings)
{
    const auto [elem_pos, elem_size] = getPosAndSizeInPixelCoords(parent_window_->size(), elem_settings.get());

    ButtonGuiElement* button = new ButtonGuiElement(parent_window_,
                                                    elem_settings,
                                                    callbacks_,
                                                    elem_pos,
                                                    elem_size);
    button->updateSizeFromParent(parent_window_->size());
    button->show();
    gui_elements_.push_back(button);
}

void WindowTab::createNewSlider(const std::shared_ptr<ElementSettings>& elem_settings)
{
    const auto [elem_pos, elem_size] = getPosAndSizeInPixelCoords(parent_window_->size(), elem_settings.get());

    SliderGuiElement* slider = new SliderGuiElement(parent_window_,
                                                    elem_settings,
                                                    callbacks_,
                                                    elem_pos,
                                                    elem_size);
    slider->updateSizeFromParent(parent_window_->size());
    slider->show();
    gui_elements_.push_back(slider);
}

void WindowTab::createNewCheckbox(const std::shared_ptr<ElementSettings>& elem_settings)
{
    const auto [elem_pos, elem_size] = getPosAndSizeInPixelCoords(parent_window_->size(), elem_settings.get());

    CheckboxGuiElement* checkbox = new CheckboxGuiElement(parent_window_,
                                                          elem_settings,
                                                          callbacks_,
                                                          elem_pos,
                                                          elem_size);
    checkbox->updateSizeFromParent(parent_window_->size());
    checkbox->show();
    gui_elements_.push_back(checkbox);
}

void WindowTab::createNewTextLabel(const std::shared_ptr<ElementSettings>& elem_settings)
{
    const auto [elem_pos, elem_size] = getPosAndSizeInPixelCoords(parent_window_->size(), elem_settings.get());

    TextLabelGuiElement* label = new TextLabelGuiElement(parent_window_,
                                                         elem_settings,
                                                         callbacks_,
                                                         elem_pos,
                                                         elem_size);
    label->updateSizeFromParent(parent_window_->size());
    label->show();
    gui_elements_.push_back(label);
}

void WindowTab::createNewListBox(const std::shared_ptr<ElementSettings>& element_settings)
{
    const auto [elem_pos, elem_size] = getPosAndSizeInPixelCoords(parent_window_->size(), element_settings.get());

    ListBoxGuiElement* list_box = new ListBoxGuiElement(parent_window_,
                                                        element_settings,
                                                        callbacks_,
                                                        elem_pos,
                                                        elem_size);
    list_box->updateSizeFromParent(parent_window_->size());
    list_box->show();
    gui_elements_.push_back(list_box);
}

void WindowTab::createNewEditableText(const std::shared_ptr<ElementSettings>& element_settings)
{
    const auto [elem_pos, elem_size] = getPosAndSizeInPixelCoords(parent_window_->size(), element_settings.get());

    EditableTextGuiElement* editable_text = new EditableTextGuiElement(parent_window_,
                                                                       element_settings,
                                                                       callbacks_,
                                                                       elem_pos,
                                                                       elem_size);
    editable_text->updateSizeFromParent(parent_window_->size());
    editable_text->show();
    gui_elements_.push_back(editable_text);
}

void WindowTab::createDropdownMenu(const std::shared_ptr<ElementSettings>& element_settings)
{
    const auto [elem_pos, elem_size] = getPosAndSizeInPixelCoords(parent_window_->size(), element_settings.get());

    DropdownMenuGuiElement* dropdown = new DropdownMenuGuiElement(parent_window_,
                                                                   element_settings,
                                                                   callbacks_,
                                                                   elem_pos,
                                                                   elem_size);
    dropdown->updateSizeFromParent(parent_window_->size());
    dropdown->show();
    gui_elements_.push_back(dropdown);
}

void WindowTab::createRadioButtonGroup(const std::shared_ptr<ElementSettings>& element_settings)
{
    const auto [elem_pos, elem_size] = getPosAndSizeInPixelCoords(parent_window_->size(), element_settings.get());

    RadioButtonGroupGuiElement* radio_group =
        new RadioButtonGroupGuiElement(parent_window_,
                                       element_settings,
                                       callbacks_,
                                       elem_pos,
                                       elem_size);
    radio_group->updateSizeFromParent(parent_window_->size());
    radio_group->show();
    gui_elements_.push_back(radio_group);
}

void WindowTab::createScrollingText(const std::shared_ptr<ElementSettings>& /*element_settings*/)
{
    LUMOS_LOG_WARNING() << "WindowTab::createScrollingText: ScrollingTextGuiElement not ported yet";
}

void WindowTab::show()
{
    for (const auto& pp : plot_panes_)
    {
        pp->show();
    }
    for (const auto& ge : gui_elements_)
    {
        ge->show();
    }
}

void WindowTab::hide()
{
    for (const auto& pp : plot_panes_)
    {
        pp->hide();
    }
    for (const auto& ge : gui_elements_)
    {
        ge->hide();
    }
}

void WindowTab::updateSizeFromParent(const QSize new_size) const
{
    for (const auto& pp : plot_panes_)
    {
        pp->updateSizeFromParent(new_size);
    }
    for (const auto& elem : gui_elements_)
    {
        elem->updateSizeFromParent(new_size);
    }
}

RGBTripletf WindowTab::getBackgroundColor() const
{
    return background_color_;
}

std::string WindowTab::getName() const
{
    return name_;
}

TabSettings WindowTab::getTabSettings() const
{
    TabSettings ts;

    ts.name = name_;
    ts.background_color = background_color_;
    ts.button_normal_color = button_normal_color_;
    ts.button_clicked_color = button_clicked_color_;
    ts.button_selected_color = button_selected_color_;
    ts.button_text_color = button_text_color_;

    for (const auto& pp : plot_panes_)
    {
        std::shared_ptr<ElementSettings> es = pp->getElementSettings();
        es->z_order = z_order_queue_.getOrderOfElement(pp->getHandleString());
        ts.elements.push_back(es);
    }

    for (const auto& ge : gui_elements_)
    {
        std::shared_ptr<ElementSettings> es = ge->getElementSettings();
        es->z_order = z_order_queue_.getOrderOfElement(ge->getHandleString());
        ts.elements.push_back(es);
    }

    return ts;
}

GuiElement* WindowTab::getGuiElement(const std::string& element_handle_string) const
{
    auto q = std::find_if(plot_panes_.begin(), plot_panes_.end(), [&element_handle_string](const GuiElement* elem) {
        return elem->getHandleString() == element_handle_string;
    });

    return (plot_panes_.end() != q) ? static_cast<GuiElement*>(*q) : nullptr;
}

void WindowTab::setMouseInteractionType(const MouseInteractionType mit)
{
    for (auto& pp : plot_panes_)
    {
        pp->setMouseInteractionType(mit);
    }
}

void WindowTab::notifyChildrenOnKeyPressed(const char key)
{
    for (const auto& pp : plot_panes_)
    {
        pp->keyPressed(key);
    }
    for (const auto& ge : gui_elements_)
    {
        ge->keyPressed(key);
    }
}

void WindowTab::notifyChildrenOnKeyReleased(const char key)
{
    for (const auto& pp : plot_panes_)
    {
        pp->keyReleased(key);
    }
    for (const auto& ge : gui_elements_)
    {
        ge->keyReleased(key);
    }
}

bool WindowTab::deleteElementIfItExists(const std::string& element_handle_string)
{
    auto q_pp = std::find_if(plot_panes_.begin(), plot_panes_.end(), [&element_handle_string](const GuiElement* e) {
        return e->getHandleString() == element_handle_string;
    });

    if (plot_panes_.end() != q_pp)
    {
        delete (*q_pp);
        z_order_queue_.eraseElement(element_handle_string);
        plot_panes_.erase(q_pp);
        callbacks_.element_deleted(element_handle_string);
        return true;
    }

    auto q_ge = std::find_if(gui_elements_.begin(), gui_elements_.end(), [&element_handle_string](const GuiElement* e) {
        return e->getHandleString() == element_handle_string;
    });

    if (gui_elements_.end() != q_ge)
    {
        delete (*q_ge);
        z_order_queue_.eraseElement(element_handle_string);
        gui_elements_.erase(q_ge);
        callbacks_.element_deleted(element_handle_string);
        return true;
    }

    return false;
}

void WindowTab::toggleProjectionMode(const std::string& element_handle_string)
{
    auto q = std::find_if(plot_panes_.begin(), plot_panes_.end(), [&element_handle_string](const PlotPane* pp) {
        return pp->getHandleString() == element_handle_string;
    });

    if (plot_panes_.end() != q)
    {
        (*q)->toggleProjectionMode();
    }
}

bool WindowTab::elementWithNameExists(const std::string& element_handle_string)
{
    auto q = std::find_if(plot_panes_.begin(), plot_panes_.end(), [&element_handle_string](const GuiElement* e) {
        return e->getHandleString() == element_handle_string;
    });

    return plot_panes_.end() != q;
}

bool WindowTab::changeNameOfElementIfElementExists(const std::string& element_handle_string,
                                                   const std::map<std::string, std::string>& new_values)
{
    auto q_pp = std::find_if(plot_panes_.begin(), plot_panes_.end(), [&element_handle_string](const GuiElement* e) {
        return e->getHandleString() == element_handle_string;
    });

    if (plot_panes_.end() != q_pp)
    {
        (*q_pp)->updateElementSettings(new_values);
        return true;
    }

    auto q_ge = std::find_if(gui_elements_.begin(), gui_elements_.end(), [&element_handle_string](const GuiElement* e) {
        return e->getHandleString() == element_handle_string;
    });

    if (gui_elements_.end() != q_ge)
    {
        (*q_ge)->updateElementSettings(new_values);
        return true;
    }

    return false;
}

bool WindowTab::raiseElement(const std::string& element_handle_string)
{
    auto q = std::find_if(plot_panes_.begin(), plot_panes_.end(), [&element_handle_string](const PlotPane* pp) {
        return pp->getHandleString() == element_handle_string;
    });

    if (plot_panes_.end() != q)
    {
        (*q)->raise();
        z_order_queue_.raise(element_handle_string);
        return true;
    }
    return false;
}

bool WindowTab::lowerElement(const std::string& element_handle_string)
{
    auto q = std::find_if(plot_panes_.begin(), plot_panes_.end(), [&element_handle_string](const PlotPane* pp) {
        return pp->getHandleString() == element_handle_string;
    });

    if (plot_panes_.end() != q)
    {
        (*q)->lower();
        z_order_queue_.lower(element_handle_string);
        return true;
    }
    return false;
}

std::vector<std::string> WindowTab::getElementNames() const
{
    std::vector<std::string> names;

    for (const auto& pp : plot_panes_)
    {
        names.push_back(pp->getHandleString());
    }
    for (const auto& ge : gui_elements_)
    {
        names.push_back(ge->getHandleString());
    }

    return names;
}

void WindowTab::setName(const std::string& new_name)
{
    name_ = new_name;
}
