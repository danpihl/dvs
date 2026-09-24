#include "gui_window.h"

#include <QActionGroup>
#include <QCloseEvent>
#include <QDateTime>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScreen>
#include <QShowEvent>

#include <algorithm>

#include "lumos/logging.h"
#include "main_window.h"
#include "settings_dialog.h"

GuiWindow::GuiWindow(
    QWidget* main_window,
    const WindowSettings& window_settings,
    const std::string& project_name,
    const int callback_id,
    const bool project_is_saved,
    const GuiCallbacks& callbacks)
    : QMainWindow(nullptr),
      callback_id_{callback_id},
      current_tab_num_{0},
      callbacks_{callbacks},
      main_window_{main_window},
      project_is_saved_{project_is_saved},
      laid_out_once_{false}
{
    project_name_ = project_name;

    // Real layout: a fixed-width tab-button strip plus an expanding content
    // area. Every WindowTab element is parented to content_area_ and sized
    // as a fraction of *its* size — Qt reserves the tab strip's space for
    // us, so no element ever needs to know it exists (see header comment).
    central_ = new QWidget(this);
    setCentralWidget(central_);

    QHBoxLayout* h_layout = new QHBoxLayout(central_);
    h_layout->setContentsMargins(0, 0, 0, 0);
    h_layout->setSpacing(0);

    tab_button_panel_ = new QWidget(central_);
    tab_button_panel_->setFixedWidth(70);
    h_layout->addWidget(tab_button_panel_);

    content_area_ = new QWidget(central_);
    content_area_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    h_layout->addWidget(content_area_);

    // Overwrites the field received from MainWindow (which is empty —
    // MainWindow never participates in right-click dispatch) with a lambda
    // wrapping this window's own mouseRightPressed(), before passing
    // callbacks_ down to any WindowTab this window creates. See
    // gui_callbacks.h.
    callbacks_.right_mouse_pressed = [this](const QPoint pos, const std::string& item_name) {
        mouseRightPressed(pos, ClickSource::GUI_ELEMENT, item_name);
    };

    name_ = window_settings.name;
    updateLabel();

    std::vector<TabSettings> tab_settings_list = window_settings.tabs;
    if (tab_settings_list.empty())
    {
        TabSettings default_tab;
        default_tab.name = "Tab " + std::to_string(current_tab_num_);
        tab_settings_list.push_back(default_tab);
    }

    for (const TabSettings& tab_settings : tab_settings_list)
    {
        WindowTab* tab = new WindowTab(content_area_, tab_settings, callbacks_);

        addTabButton(tab);

        current_tab_num_++;
    }

    for (size_t i = 0; i < tab_entries_.size(); i++)
    {
        if (i == 0)
        {
            tab_entries_[i].tab->show();
            const RGBTripletf c = tab_entries_[i].tab->getBackgroundColor();
            QPalette pal = content_area_->palette();
            pal.setColor(QPalette::Window, QColor::fromRgbF(c.red, c.green, c.blue));
            content_area_->setAutoFillBackground(true);
            content_area_->setPalette(pal);
            tab_entries_[i].button->setChecked(true);
        }
        else
        {
            tab_entries_[i].tab->hide();
        }
    }

    layoutTabButtons();
    buildPopupMenus();

    setGeometry(window_settings.x, window_settings.y, window_settings.width, window_settings.height);

    // Defensive, matching the fix for the old attempt's "Bug A": don't rely
    // solely on whatever resizeEvent(s) setGeometry() happens to trigger —
    // explicitly bring every tab's elements up to date against the real
    // final size right now, on a freshly-constructed, never-yet-shown
    // window (exactly the state that bug hid in).
    for (const auto& entry : tab_entries_)
    {
        entry.tab->updateSizeFromParent(content_area_->size());
    }

    LUMOS_LOG_INFO() << "GuiWindow '" << name_ << "' created with " << tab_entries_.size() << " tabs";
}

GuiWindow::~GuiWindow()
{
    for (const auto& entry : tab_entries_)
    {
        delete entry.tab;
    }
}

void GuiWindow::deleteAllTabs()
{
    for (const auto& entry : tab_entries_)
    {
        delete entry.tab;
    }
    tab_entries_.clear();
}

void GuiWindow::setProjectName(const std::string& project_name)
{
    project_name_ = project_name;
}

std::vector<GuiElement*> GuiWindow::getGuiElements() const
{
    std::vector<GuiElement*> elems;
    for (const auto& entry : tab_entries_)
    {
        const auto tab_elems = entry.tab->getGuiElements();
        elems.insert(elems.end(), tab_elems.begin(), tab_elems.end());
    }
    return elems;
}

std::vector<GuiElement*> GuiWindow::getPlotPanes() const
{
    std::vector<GuiElement*> elems;
    for (const auto& entry : tab_entries_)
    {
        const auto tab_elems = entry.tab->getPlotPanes();
        elems.insert(elems.end(), tab_elems.begin(), tab_elems.end());
    }
    return elems;
}

std::vector<GuiElement*> GuiWindow::getAllGuiElements() const
{
    std::vector<GuiElement*> elems;
    for (const auto& entry : tab_entries_)
    {
        const auto tab_elems = entry.tab->getAllGuiElements();
        elems.insert(elems.end(), tab_elems.begin(), tab_elems.end());
    }
    return elems;
}

GuiElement* GuiWindow::getGuiElement(const std::string& element_handle_string) const
{
    for (const auto& entry : tab_entries_)
    {
        GuiElement* ge = entry.tab->getGuiElement(element_handle_string);
        if (ge != nullptr)
        {
            return ge;
        }
    }
    return nullptr;
}

void GuiWindow::keyPressEvent(QKeyEvent* event)
{
    notifyChildrenOnKeyPressed(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
}

void GuiWindow::keyReleaseEvent(QKeyEvent* event)
{
    notifyChildrenOnKeyReleased(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
}

void GuiWindow::notifyChildrenOnKeyPressed(const char key)
{
    // HelpPane's 'H' toggle is still out of scope (native-chrome decision).
    if (key == 'r' || key == 'R')
    {
        checkModeInAllMenus("Rotate");
    }
    else if (key == 'z' || key == 'Z')
    {
        checkModeInAllMenus("Zoom");
    }
    else if (key == 'p' || key == 'P')
    {
        checkModeInAllMenus("Pan");
    }

    for (const auto& entry : tab_entries_)
    {
        entry.tab->notifyChildrenOnKeyPressed(key);
    }
}

void GuiWindow::notifyChildrenOnKeyReleased(const char key)
{
    for (const auto& entry : tab_entries_)
    {
        entry.tab->notifyChildrenOnKeyReleased(key);
    }
}

void GuiWindow::mouseRightPressed(const QPoint pos, const ClickSource source, const std::string& item_name)
{
    last_clicked_item_ = item_name;

    // `pos`'s coordinate space depends on `source` — each call site passes
    // it relative to whichever widget it originated from (content_area_ for
    // GUI_ELEMENT, this GuiWindow itself for THIS, tab_button_panel_ for
    // TAB_BUTTON — see gui_element.cpp's mouseRightReleased and
    // addTabButton above), so the matching widget maps it to global here.
    if (source == ClickSource::GUI_ELEMENT)
    {
        popup_menu_element_->popup(content_area_->mapToGlobal(pos));
    }
    else if (source == ClickSource::TAB_BUTTON)
    {
        popup_menu_tab_->popup(tab_button_panel_->mapToGlobal(pos));
    }
    else
    {
        popup_menu_window_->popup(this->mapToGlobal(pos));
    }
}

void GuiWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        mouseRightPressed(event->pos(), ClickSource::THIS, "");
    }
}

void GuiWindow::tabChanged(const std::string& name)
{
    for (const auto& entry : tab_entries_)
    {
        if (entry.tab->getName() == name)
        {
            entry.tab->show();
            const RGBTripletf c = entry.tab->getBackgroundColor();
            QPalette pal = content_area_->palette();
            pal.setColor(QPalette::Window, QColor::fromRgbF(c.red, c.green, c.blue));
            content_area_->setAutoFillBackground(true);
            content_area_->setPalette(pal);
            entry.button->setChecked(true);
            content_area_->update();
        }
        else
        {
            entry.tab->hide();
            entry.button->setChecked(false);
        }
    }
}

void GuiWindow::layoutTabButtons()
{
    // Hidden entirely with a single tab, matching wx.
    tab_button_panel_->setVisible(tab_entries_.size() > 1);

    for (size_t k = 0; k < tab_entries_.size(); k++)
    {
        tab_entries_[k].button->setGeometry(0, static_cast<int>(k) * 30, 70, 30);
        // A newly created child only inherits visibility from the
        // hidden->visible transition of its parent at the moment that
        // transition happens — a button added *after* tab_button_panel_ is
        // already visible (i.e. every tab from the 3rd onward) needs an
        // explicit show(), exactly like MainWindow::layoutWindowButtons()
        // already does for its own per-window buttons.
        tab_entries_[k].button->show();
    }
}

void GuiWindow::addTabButton(WindowTab* tab)
{
    // Captures `tab` (a stable pointer), not an index — see the TabEntry
    // comment in gui_window.h/ARCHITECTURE_IMPROVEMENTS.md #1.
    QPushButton* btn = new QPushButton(QString::fromStdString(tab->getName()), tab_button_panel_);
    btn->setCheckable(true);
    connect(btn, &QPushButton::clicked, this, [this, tab]() { tabChanged(tab->getName()); });

    btn->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(btn, &QPushButton::customContextMenuRequested, this, [this, btn, tab](const QPoint& local_pos) {
        mouseRightPressed(btn->mapTo(tab_button_panel_, local_pos), ClickSource::TAB_BUTTON, tab->getName());
    });

    tab_entries_.push_back(TabEntry{tab, btn});
}

void GuiWindow::moveEvent(QMoveEvent* event)
{
    QMainWindow::moveEvent(event);
    callbacks_.about_modification();
}

void GuiWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    layoutTabButtons();

    // Matches wx exactly (main_application/gui_window.cpp:498-513): every
    // tab is kept in sync on every resize, not only the currently-visible
    // one — a hidden tab must already have correct geometry the moment it's
    // switched to, not get it lazily on switchToTab.
    for (const auto& entry : tab_entries_)
    {
        entry.tab->updateSizeFromParent(content_area_->size());
    }

    callbacks_.about_modification();
}

void GuiWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    // The real fix for a bug the constructor's own "defensive" pass
    // couldn't fully solve: at construction time the window has never been
    // shown, and content_area_'s QHBoxLayout may not have actually
    // recalculated its child geometry yet even after setGeometry() —
    // resulting in every element being sized against a stale, tiny,
    // pre-layout content_area_ size (confirmed by the user: every element
    // rendered "super small"). wx sidesteps this by calling Show() *before*
    // creating any tabs/elements, then relying on a later SetSize() call to
    // fire a real wxSizeEvent that re-lays-out everything against the
    // now-guaranteed-correct size (see OnSize). This does the Qt
    // equivalent: by the time showEvent fires, Qt has definitely finished
    // laying out content_area_, so this is the first point at which it's
    // safe to trust its size. Runs once, the first time the window is
    // actually shown.
    if (!laid_out_once_)
    {
        laid_out_once_ = true;
        layoutTabButtons();
        for (const auto& entry : tab_entries_)
        {
            entry.tab->updateSizeFromParent(content_area_->size());
        }
    }
}

void GuiWindow::closeEvent(QCloseEvent* event)
{
    callbacks_.about_modification();
    hide();
    event->ignore();
}

int GuiWindow::getCallbackId() const
{
    return callback_id_;
}

void GuiWindow::screenshot(const std::string& base_path)
{
    raise();

    QScreen* screen = this->screen();
    if (screen == nullptr)
    {
        LUMOS_LOG_WARNING() << "screenshot: no screen available";
        return;
    }

    const QPixmap pixmap = screen->grabWindow(this->winId());

    const QString timestamp = QDateTime::currentDateTime().toString("dd-MM-yyyy HH-mm-ss");
    const QString file_name =
        QString::fromStdString(base_path) + "Screenshot_" + timestamp + "_" + QString::fromStdString(name_) + ".png";

    pixmap.save(file_name, "PNG");
}

void GuiWindow::updateLabel()
{
    setWindowTitle(QString::fromStdString(name_));
}

void GuiWindow::setName(const std::string& new_name)
{
    callbacks_.name_changed(name_, new_name);
    name_ = new_name;
    updateLabel();
}

WindowSettings GuiWindow::getWindowSettings() const
{
    WindowSettings ws;
    ws.name = name_;
    ws.x = x();
    ws.y = y();
    ws.width = width();
    ws.height = height();

    for (const auto& entry : tab_entries_)
    {
        ws.tabs.push_back(entry.tab->getTabSettings());
    }

    return ws;
}

std::string GuiWindow::getName() const
{
    return name_;
}

void GuiWindow::setIsFileSavedForLabel(const bool is_saved)
{
    project_is_saved_ = is_saved;
    updateLabel();
}

void GuiWindow::createNewPlotPane()
{
    if (!tab_entries_.empty())
    {
        const size_t idx = current_tab_num_ < static_cast<int>(tab_entries_.size())
                               ? static_cast<size_t>(current_tab_num_)
                               : 0;
        tab_entries_[idx].tab->createNewPlotPane();
    }
}

void GuiWindow::createNewPlotPane(const std::string& handle_string)
{
    if (!tab_entries_.empty())
    {
        tab_entries_[0].tab->createNewPlotPane(handle_string);
    }
}

void GuiWindow::updateAllElements()
{
    for (const auto& entry : tab_entries_)
    {
        entry.tab->updateAllElements();
    }
}

std::vector<std::string> GuiWindow::getElementNames() const
{
    std::vector<std::string> names;
    for (const auto& entry : tab_entries_)
    {
        const auto tab_names = entry.tab->getElementNames();
        names.insert(names.end(), tab_names.begin(), tab_names.end());
    }
    return names;
}

// ---------------------------------------------------------------------------
// Popup-menu system — port of main_application/gui_window.cpp:134-1286's
// menu construction and dispatch (see the class comment in gui_window.h).
// ---------------------------------------------------------------------------

WindowTab* GuiWindow::getSelectedTab() const
{
    for (const auto& entry : tab_entries_)
    {
        if (entry.button->isChecked())
        {
            return entry.tab;
        }
    }
    return tab_entries_.empty() ? nullptr : tab_entries_[0].tab;
}

QAction* GuiWindow::getMenuActionByText(QMenu* menu, const std::string& text) const
{
    const QList<QAction*> actions = menu->actions();
    for (QAction* action : actions)
    {
        if (action->text().toStdString() == text)
        {
            return action;
        }
    }
    return nullptr;
}

void GuiWindow::checkModeInAllMenus(const std::string& text)
{
    for (QMenu* menu : {popup_menu_window_, popup_menu_element_, popup_menu_tab_})
    {
        QAction* action = getMenuActionByText(menu, text);
        if (action != nullptr)
        {
            action->setChecked(true);
        }
    }
}

void GuiWindow::setMouseInteractionModeForAllTabs(const std::string& mode)
{
    MouseInteractionType mit = MouseInteractionType::ROTATE;
    if (mode == "Zoom")
    {
        mit = MouseInteractionType::ZOOM;
    }
    else if (mode == "Pan")
    {
        mit = MouseInteractionType::PAN;
    }
    else if (mode == "Select")
    {
        mit = MouseInteractionType::POINT_SELECTION;
    }

    for (const auto& entry : tab_entries_)
    {
        entry.tab->setMouseInteractionType(mit);
    }
}

void GuiWindow::addModeRadioGroup(QMenu* menu)
{
    QActionGroup* group = new QActionGroup(this);
    for (const std::string& mode : {std::string("Zoom"), std::string("Pan"), std::string("Rotate"), std::string("Select")})
    {
        QAction* action = menu->addAction(QString::fromStdString(mode));
        action->setCheckable(true);
        group->addAction(action);
        if (mode == "Rotate")
        {
            action->setChecked(true);
        }
        connect(action, &QAction::triggered, this, [this, mode]() { setMouseInteractionModeForAllTabs(mode); });
    }
}

void GuiWindow::buildPopupMenus()
{
    new_element_menu_window_ = new QMenu("New element", this);
    new_element_menu_element_ = new QMenu("New element", this);
    new_element_menu_tab_ = new QMenu("New element", this);
    popup_menu_window_ = new QMenu(this);
    popup_menu_element_ = new QMenu(this);
    popup_menu_tab_ = new QMenu(this);

    // Matches wx's items list exactly (9 entries), but only 5 are wired to
    // a working handler — the other 4 are empty handler stubs in wx itself
    // (main_application/gui_window.cpp:755-794), so they're present but
    // disabled here too. See the class comment in gui_window.h.
    struct NewElementItem
    {
        std::string label;
        std::function<void()> handler;
    };
    const std::vector<NewElementItem> items{
        {"Plot pane", [this]() { createNewPlotPaneCallback(); }},
        {"Button", [this]() { createNewButtonCallback(); }},
        {"Slider", [this]() { createNewSliderCallback(); }},
        {"List box", nullptr},
        {"Editable Text", nullptr},
        {"Drop Down Menu", nullptr},
        {"Radio Button Group", nullptr},
        {"Checkbox", [this]() { createNewCheckboxCallback(); }},
        {"Text label", [this]() { createNewTextLabelCallback(); }}};

    for (QMenu* submenu : {new_element_menu_window_, new_element_menu_element_, new_element_menu_tab_})
    {
        for (const NewElementItem& item : items)
        {
            QAction* action = submenu->addAction(QString::fromStdString(item.label));
            if (item.handler)
            {
                connect(action, &QAction::triggered, this, item.handler);
            }
            else
            {
                action->setEnabled(false);
            }
        }
    }

    // popup_menu_window_ — main_application/gui_window.cpp:159-175.
    popup_menu_window_->addSeparator();
    connect(popup_menu_window_->addAction("Edit window name"), &QAction::triggered, this,
            [this]() { editWindowName(); });
    connect(popup_menu_window_->addAction("Delete window"), &QAction::triggered, this, [this]() {
        static_cast<MainWindow*>(main_window_)->deleteWindow(callback_id_);
    });
    popup_menu_window_->addSeparator();
    addModeRadioGroup(popup_menu_window_);
    popup_menu_window_->addSeparator();
    connect(popup_menu_window_->addAction("Print gui code"), &QAction::triggered, this, [this]() { printGuiCode(); });
    popup_menu_window_->addSeparator();
    connect(popup_menu_window_->addAction("New window"), &QAction::triggered, this,
            [this]() { static_cast<MainWindow*>(main_window_)->newWindow(); });
    connect(popup_menu_window_->addAction("New tab"), &QAction::triggered, this, [this]() { newTab(); });
    popup_menu_window_->addMenu(new_element_menu_window_);

    // popup_menu_element_ — main_application/gui_window.cpp:177-197.
    connect(popup_menu_element_->addAction("Edit element"), &QAction::triggered, this,
            [this]() { editElementName(); });
    connect(popup_menu_element_->addAction("Delete element"), &QAction::triggered, this,
            [this]() { deleteElementAction(); });
    connect(popup_menu_element_->addAction("Toggle projection mode"), &QAction::triggered, this,
            [this]() { toggleProjectionModeAction(); });
    connect(popup_menu_element_->addAction("Raise"), &QAction::triggered, this, [this]() { raiseElementAction(); });
    connect(popup_menu_element_->addAction("Lower"), &QAction::triggered, this, [this]() { lowerElementAction(); });
    popup_menu_element_->addSeparator();
    addModeRadioGroup(popup_menu_element_);
    popup_menu_element_->addSeparator();
    connect(popup_menu_element_->addAction("Print gui code"), &QAction::triggered, this,
            [this]() { printGuiCode(); });
    popup_menu_element_->addSeparator();
    connect(popup_menu_element_->addAction("Edit window name"), &QAction::triggered, this,
            [this]() { editWindowName(); });
    connect(popup_menu_element_->addAction("Delete window"), &QAction::triggered, this, [this]() {
        static_cast<MainWindow*>(main_window_)->deleteWindow(callback_id_);
    });
    popup_menu_element_->addSeparator();
    connect(popup_menu_element_->addAction("New window"), &QAction::triggered, this,
            [this]() { static_cast<MainWindow*>(main_window_)->newWindow(); });
    connect(popup_menu_element_->addAction("New tab"), &QAction::triggered, this, [this]() { newTab(); });
    popup_menu_element_->addMenu(new_element_menu_element_);

    // popup_menu_tab_ — main_application/gui_window.cpp:199-216.
    connect(popup_menu_tab_->addAction("Edit tab name"), &QAction::triggered, this, [this]() { editTabName(); });
    connect(popup_menu_tab_->addAction("Delete tab"), &QAction::triggered, this, [this]() { deleteTab(); });
    popup_menu_tab_->addSeparator();
    addModeRadioGroup(popup_menu_tab_);
    popup_menu_tab_->addSeparator();
    connect(popup_menu_tab_->addAction("Print gui code"), &QAction::triggered, this, [this]() { printGuiCode(); });
    popup_menu_tab_->addSeparator();
    connect(popup_menu_tab_->addAction("Edit window name"), &QAction::triggered, this,
            [this]() { editWindowName(); });
    connect(popup_menu_tab_->addAction("Delete window"), &QAction::triggered, this, [this]() {
        static_cast<MainWindow*>(main_window_)->deleteWindow(callback_id_);
    });
    popup_menu_tab_->addSeparator();
    connect(popup_menu_tab_->addAction("New window"), &QAction::triggered, this,
            [this]() { static_cast<MainWindow*>(main_window_)->newWindow(); });
    connect(popup_menu_tab_->addAction("New tab"), &QAction::triggered, this, [this]() { newTab(); });
    popup_menu_tab_->addMenu(new_element_menu_tab_);
}

std::map<std::string, std::string> GuiWindow::getValidNewElementHandleString(
    const std::map<std::string, std::pair<std::string, std::string>>& fields)
{
    std::map<std::string, std::string> ret_fields;

    while (true)
    {
        SettingsDialog dialog(this, "Enter element settings", fields);

        if (dialog.exec() == QDialog::Rejected)
        {
            ret_fields["handle_string"] = "";
            return ret_fields;
        }

        const std::string element_handle_string = dialog.getFieldString("handle_string");

        if (element_handle_string.empty())
        {
            QMessageBox::warning(this, "Invalid name!", "Can't have an empty element name!");
            continue;
        }

        const std::vector<std::string> all_element_names = callbacks_.get_all_element_names();
        const bool element_exists =
            std::find(all_element_names.begin(), all_element_names.end(), element_handle_string) !=
            all_element_names.end();

        if (element_exists)
        {
            QMessageBox::warning(
                this, "Element name \"" + QString::fromStdString(element_handle_string) + "\" exists!",
                "Choose a unique name");
            continue;
        }

        for (const auto& p : fields)
        {
            ret_fields[p.first] = dialog.getFieldString(p.first);
        }
        return ret_fields;
    }
}

std::shared_ptr<ElementSettings> GuiWindow::findElementSettings(const std::string& handle_string) const
{
    for (const auto& entry : tab_entries_)
    {
        for (GuiElement* ge : entry.tab->getGuiElements())
        {
            if (ge->getHandleString() == handle_string)
            {
                return ge->getElementSettings();
            }
        }
        for (GuiElement* pp : entry.tab->getPlotPanes())
        {
            if (pp->getHandleString() == handle_string)
            {
                return pp->getElementSettings();
            }
        }
    }
    return nullptr;
}

void GuiWindow::editWindowName()
{
    bool ok = false;
    const QString new_name = QInputDialog::getText(this, "Enter window name", "Enter the name for the window",
                                                     QLineEdit::Normal, QString::fromStdString(name_), &ok);
    if (!ok || new_name.isEmpty())
    {
        return;
    }

    setName(new_name.toStdString());
    callbacks_.about_modification();
}

void GuiWindow::newTab()
{
    TabSettings tab_settings;
    tab_settings.name = "Tab " + std::to_string(current_tab_num_);
    current_tab_num_++;

    WindowTab* tab = new WindowTab(content_area_, tab_settings, callbacks_);
    tab->hide();
    addTabButton(tab);

    layoutTabButtons();
    callbacks_.about_modification();
}

void GuiWindow::editTabName()
{
    bool ok = false;
    const QString new_name = QInputDialog::getText(this, "Enter tab name", "Enter the new name for tab",
                                                     QLineEdit::Normal, QString::fromStdString(last_clicked_item_),
                                                     &ok);
    if (!ok || new_name.isEmpty())
    {
        return;
    }

    const auto q = std::find_if(tab_entries_.begin(), tab_entries_.end(),
                                 [this](const TabEntry& entry) { return last_clicked_item_ == entry.tab->getName(); });
    if (q == tab_entries_.end())
    {
        return;
    }

    q->tab->setName(new_name.toStdString());
    q->button->setText(new_name);
    callbacks_.about_modification();
}

void GuiWindow::deleteTab()
{
    const auto q = std::find_if(tab_entries_.begin(), tab_entries_.end(),
                                 [this](const TabEntry& entry) { return last_clicked_item_ == entry.tab->getName(); });
    if (q == tab_entries_.end())
    {
        return;
    }

    // wx re-derives which tab should become visible from its TabButtons'
    // own internal selection bookkeeping after the delete; here it's
    // simpler and equivalent to check *before* deleting whether the tab
    // being removed was the visible one, and fall back to tab_entries_[0]
    // if so.
    const bool was_selected = q->button->isChecked();

    delete q->tab;
    q->button->deleteLater();
    tab_entries_.erase(q);

    if (!tab_entries_.empty() && was_selected)
    {
        tabChanged(tab_entries_[0].tab->getName());
    }

    layoutTabButtons();
    callbacks_.about_modification();
}

void GuiWindow::createNewPlotPaneCallback()
{
    WindowTab* tab = getSelectedTab();
    if (tab == nullptr)
    {
        return;
    }

    std::map<std::string, std::pair<std::string, std::string>> fields;
    fields["handle_string"] = {"Handle string", ""};
    fields["title"] = {"Title", ""};

    const std::map<std::string, std::string> ret_fields = getValidNewElementHandleString(fields);
    const std::string element_handle_string = ret_fields.at("handle_string");

    if (!element_handle_string.empty())
    {
        const std::shared_ptr<PlotPaneSettings> pp_settings = std::make_shared<PlotPaneSettings>();
        pp_settings->handle_string = element_handle_string;
        pp_settings->title = ret_fields.at("title");

        tab->createNewPlotPane(pp_settings);
        callbacks_.about_modification();
    }
}

void GuiWindow::createNewButtonCallback()
{
    WindowTab* tab = getSelectedTab();
    if (tab == nullptr)
    {
        return;
    }

    std::map<std::string, std::pair<std::string, std::string>> fields;
    fields["handle_string"] = {"Handle string", ""};
    fields["label"] = {"Label", ""};

    const std::map<std::string, std::string> ret_fields = getValidNewElementHandleString(fields);
    const std::string element_handle_string = ret_fields.at("handle_string");

    if (!element_handle_string.empty())
    {
        const std::shared_ptr<ButtonSettings> elem_settings = std::make_shared<ButtonSettings>();
        elem_settings->handle_string = element_handle_string;
        elem_settings->label = ret_fields.at("label");

        tab->createNewButton(elem_settings);
        callbacks_.about_modification();
    }
}

void GuiWindow::createNewSliderCallback()
{
    WindowTab* tab = getSelectedTab();
    if (tab == nullptr)
    {
        return;
    }

    std::map<std::string, std::pair<std::string, std::string>> fields;
    fields["handle_string"] = {"Handle string", ""};
    fields["min_value"] = {"Min value", "0"};
    fields["max_value"] = {"Max value", "100"};
    fields["step_size"] = {"Step size", "1"};

    const std::map<std::string, std::string> ret_fields = getValidNewElementHandleString(fields);
    const std::string element_handle_string = ret_fields.at("handle_string");

    if (!element_handle_string.empty())
    {
        const std::shared_ptr<SliderSettings> elem_settings = std::make_shared<SliderSettings>();
        elem_settings->handle_string = element_handle_string;
        elem_settings->min_value = std::stoi(ret_fields.at("min_value"));
        elem_settings->max_value = std::stoi(ret_fields.at("max_value"));
        elem_settings->step_size = std::stoi(ret_fields.at("step_size"));
        elem_settings->init_value = elem_settings->min_value;

        tab->createNewSlider(elem_settings);
        callbacks_.about_modification();
    }
}

void GuiWindow::createNewCheckboxCallback()
{
    WindowTab* tab = getSelectedTab();
    if (tab == nullptr)
    {
        return;
    }

    std::map<std::string, std::pair<std::string, std::string>> fields;
    fields["handle_string"] = {"Handle string", ""};
    fields["label"] = {"Label", ""};

    const std::map<std::string, std::string> ret_fields = getValidNewElementHandleString(fields);
    const std::string element_handle_string = ret_fields.at("handle_string");

    if (!element_handle_string.empty())
    {
        const std::shared_ptr<CheckboxSettings> elem_settings = std::make_shared<CheckboxSettings>();
        elem_settings->handle_string = element_handle_string;
        elem_settings->label = ret_fields.at("label");

        tab->createNewCheckbox(elem_settings);
        callbacks_.about_modification();
    }
}

void GuiWindow::createNewTextLabelCallback()
{
    WindowTab* tab = getSelectedTab();
    if (tab == nullptr)
    {
        return;
    }

    std::map<std::string, std::pair<std::string, std::string>> fields;
    fields["handle_string"] = {"Handle string", ""};
    fields["label"] = {"Label", ""};

    const std::map<std::string, std::string> ret_fields = getValidNewElementHandleString(fields);
    const std::string element_handle_string = ret_fields.at("handle_string");

    if (!element_handle_string.empty())
    {
        const std::shared_ptr<TextLabelSettings> elem_settings = std::make_shared<TextLabelSettings>();
        elem_settings->handle_string = element_handle_string;
        elem_settings->label = ret_fields.at("label");

        tab->createNewTextLabel(elem_settings);
        callbacks_.about_modification();
    }
}

namespace
{
// Port of the free function of the same name in
// main_application/gui_window.cpp:987-1018.
std::map<std::string, std::pair<std::string, std::string>> transformElementSettingsToFieldsMap(
    const std::shared_ptr<ElementSettings>& element_settings)
{
    std::map<std::string, std::pair<std::string, std::string>> ret_fields;
    ret_fields["handle_string"] = {"Handle string", element_settings->handle_string};

    if (element_settings->type == lumos::GuiElementType::Button)
    {
        const auto bs = std::dynamic_pointer_cast<ButtonSettings>(element_settings);
        ret_fields["label"] = {"Label", bs->label};
    }
    else if (element_settings->type == lumos::GuiElementType::Checkbox)
    {
        const auto cs = std::dynamic_pointer_cast<CheckboxSettings>(element_settings);
        ret_fields["label"] = {"Label", cs->label};
    }
    else if (element_settings->type == lumos::GuiElementType::Slider)
    {
        const auto ss = std::dynamic_pointer_cast<SliderSettings>(element_settings);
        ret_fields["step_size"] = {"Step size", std::to_string(ss->step_size)};
        ret_fields["max_value"] = {"Max value", std::to_string(ss->max_value)};
        ret_fields["min_value"] = {"Min value", std::to_string(ss->min_value)};
    }
    else if (element_settings->type == lumos::GuiElementType::PlotPane)
    {
        const auto pps = std::dynamic_pointer_cast<PlotPaneSettings>(element_settings);
        ret_fields["title"] = {"Title", pps->title};
    }

    return ret_fields;
}
}  // namespace

void GuiWindow::editElementName()
{
    const std::shared_ptr<ElementSettings> element_settings = findElementSettings(last_clicked_item_);

    if (element_settings == nullptr)
    {
        LUMOS_LOG_WARNING() << "Couldn't find element with name " << last_clicked_item_ << " to edit";
        return;
    }

    const std::map<std::string, std::pair<std::string, std::string>> fields =
        transformElementSettingsToFieldsMap(element_settings);

    std::map<std::string, std::string> ret_fields;

    while (true)
    {
        SettingsDialog dialog(this, "Enter element settings", fields);

        if (dialog.exec() == QDialog::Rejected)
        {
            return;
        }

        const std::string element_handle_string = dialog.getFieldString("handle_string");

        if (element_handle_string.empty())
        {
            QMessageBox::warning(this, "Invalid name!", "Can't have an empty element name!");
            continue;
        }

        bool name_ok = element_handle_string == last_clicked_item_;

        if (!name_ok)
        {
            const std::vector<std::string> all_element_names = callbacks_.get_all_element_names();
            const bool element_exists =
                std::find(all_element_names.begin(), all_element_names.end(), element_handle_string) !=
                all_element_names.end();

            if (element_exists)
            {
                QMessageBox::warning(
                    this, "Element name \"" + QString::fromStdString(element_handle_string) + "\" exists!",
                    "Choose a unique name");
                continue;
            }
            name_ok = true;
        }

        // Populated here, while `dialog` is still in scope, regardless of
        // which of the two ok paths above was taken (matches wx: it fills
        // ret_fields from the dialog once, after the loop, for either case).
        for (const auto& p : fields)
        {
            ret_fields[p.first] = dialog.getFieldString(p.first);
        }
        break;
    }

    bool name_changed = false;
    for (const auto& entry : tab_entries_)
    {
        name_changed = entry.tab->changeNameOfElementIfElementExists(last_clicked_item_, ret_fields) || name_changed;
    }

    if (name_changed)
    {
        callbacks_.element_name_changed(last_clicked_item_, ret_fields["handle_string"]);
    }
}

void GuiWindow::deleteElementAction()
{
    bool element_deleted = false;
    for (const auto& entry : tab_entries_)
    {
        element_deleted = entry.tab->deleteElementIfItExists(last_clicked_item_) || element_deleted;
    }

    if (element_deleted)
    {
        callbacks_.about_modification();
    }
}

void GuiWindow::toggleProjectionModeAction()
{
    for (const auto& entry : tab_entries_)
    {
        entry.tab->toggleProjectionMode(last_clicked_item_);
    }
    callbacks_.about_modification();
}

void GuiWindow::raiseElementAction()
{
    for (const auto& entry : tab_entries_)
    {
        entry.tab->raiseElement(last_clicked_item_);
    }
    callbacks_.about_modification();
}

void GuiWindow::lowerElementAction()
{
    for (const auto& entry : tab_entries_)
    {
        entry.tab->lowerElement(last_clicked_item_);
    }
    callbacks_.about_modification();
}

void GuiWindow::printGuiCode()
{
    static_cast<MainWindow*>(main_window_)->printGuiCallbackCode();
}
