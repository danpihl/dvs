#include "gui_elements.h"

ButtonGuiElement::ButtonGuiElement(
    QWidget* parent,
    const std::shared_ptr<ElementSettings>& element_settings,
    const GuiCallbacks& callbacks,
    const QPoint& pos,
    const QSize& size)
    : QPushButton(QString::fromStdString(std::dynamic_pointer_cast<ButtonSettings>(element_settings)->label), parent),
      GuiElement(element_settings, callbacks),
      is_pressed_(false)
{
    publish_to_local_ = std::dynamic_pointer_cast<ButtonSettings>(element_settings)->publish_to_local;
    publish_to_serial_ = std::dynamic_pointer_cast<ButtonSettings>(element_settings)->publish_to_serial;
    id_ = std::dynamic_pointer_cast<ButtonSettings>(element_settings)->id;

    move(pos);
    resize(size);

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(this, &QPushButton::clicked, this, &ButtonGuiElement::buttonClicked);
}

void ButtonGuiElement::buttonClicked()
{
    if (publish_to_local_)
    {
        sendGuiData();
    }
}

void ButtonGuiElement::setLabel(const std::string& new_label)
{
    QMetaObject::invokeMethod(
        this, [this, new_label]() { QPushButton::setText(QString::fromStdString(new_label)); }, Qt::QueuedConnection);
}

void ButtonGuiElement::updateElementSettings(const std::map<std::string, std::string>& new_settings)
{
    ButtonSettings* button_settings = dynamic_cast<ButtonSettings*>(element_settings_.get());

    if (new_settings.count("label") > 0U)
    {
        setText(QString::fromStdString(new_settings.at("label")));
        button_settings->label = new_settings.at("label");
    }
    element_settings_->handle_string = new_settings.at("handle_string");
}

void ButtonGuiElement::mousePressEvent(QMouseEvent* event)
{
    mouseLeftPressed(event);
    QPushButton::mousePressEvent(event);
}

void ButtonGuiElement::mouseMoveEvent(QMouseEvent* event)
{
    mouseMovedOverItem(event);
    QPushButton::mouseMoveEvent(event);
}

void ButtonGuiElement::mouseReleaseEvent(QMouseEvent* event)
{
    mouseLeftReleased(event);
    QPushButton::mouseReleaseEvent(event);
}

void ButtonGuiElement::keyPressEvent(QKeyEvent* event)
{
    keyPressedCallback(event);
    keyPressed(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QPushButton::keyPressEvent(event);
}

void ButtonGuiElement::keyReleaseEvent(QKeyEvent* event)
{
    keyReleasedCallback(event);
    keyReleased(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QPushButton::keyReleaseEvent(event);
}

void ButtonGuiElement::enterEvent(QEnterEvent* event)
{
    mouseEnteredElement(event);
    QPushButton::enterEvent(event);
}

void ButtonGuiElement::leaveEvent(QEvent* event)
{
    mouseLeftElement(event);
    QPushButton::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// CheckboxGuiElement
// ---------------------------------------------------------------------------

CheckboxGuiElement::CheckboxGuiElement(
    QWidget* parent,
    const std::shared_ptr<ElementSettings>& element_settings,
    const GuiCallbacks& callbacks,
    const QPoint& pos,
    const QSize& size)
    : QCheckBox(QString::fromStdString(std::dynamic_pointer_cast<CheckboxSettings>(element_settings)->label), parent),
      GuiElement(element_settings, callbacks)
{
    move(pos);
    resize(size);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(this, &QCheckBox::toggled, this, &CheckboxGuiElement::checkboxToggled);
}

void CheckboxGuiElement::checkboxToggled()
{
    // wx quirk, preserved: unconditional, doesn't check publish_to_local —
    // see the class comment in gui_elements.h.
    sendGuiData();
}

void CheckboxGuiElement::mousePressEvent(QMouseEvent* event)
{
    mouseLeftPressed(event);
    QCheckBox::mousePressEvent(event);
}

void CheckboxGuiElement::mouseMoveEvent(QMouseEvent* event)
{
    mouseMovedOverItem(event);
    QCheckBox::mouseMoveEvent(event);
}

void CheckboxGuiElement::mouseReleaseEvent(QMouseEvent* event)
{
    mouseLeftReleased(event);
    QCheckBox::mouseReleaseEvent(event);
}

void CheckboxGuiElement::keyPressEvent(QKeyEvent* event)
{
    keyPressedCallback(event);
    keyPressed(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QCheckBox::keyPressEvent(event);
}

void CheckboxGuiElement::keyReleaseEvent(QKeyEvent* event)
{
    keyReleasedCallback(event);
    keyReleased(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QCheckBox::keyReleaseEvent(event);
}

void CheckboxGuiElement::enterEvent(QEnterEvent* event)
{
    mouseEnteredElement(event);
    QCheckBox::enterEvent(event);
}

void CheckboxGuiElement::leaveEvent(QEvent* event)
{
    mouseLeftElement(event);
    QCheckBox::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// TextLabelGuiElement
// ---------------------------------------------------------------------------

TextLabelGuiElement::TextLabelGuiElement(
    QWidget* parent,
    const std::shared_ptr<ElementSettings>& element_settings,
    const GuiCallbacks& callbacks,
    const QPoint& pos,
    const QSize& size)
    : QLabel(QString::fromStdString(std::dynamic_pointer_cast<TextLabelSettings>(element_settings)->label), parent),
      GuiElement(element_settings, callbacks)
{
    move(pos);
    resize(size);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

void TextLabelGuiElement::setLabel(const std::string& new_label)
{
    QMetaObject::invokeMethod(
        this, [this, new_label]() { QLabel::setText(QString::fromStdString(new_label)); }, Qt::QueuedConnection);
}

void TextLabelGuiElement::updateElementSettings(const std::map<std::string, std::string>& new_settings)
{
    TextLabelSettings* label_settings = dynamic_cast<TextLabelSettings*>(element_settings_.get());

    if (new_settings.count("label") > 0U)
    {
        setText(QString::fromStdString(new_settings.at("label")));
        label_settings->label = new_settings.at("label");
    }
    element_settings_->handle_string = new_settings.at("handle_string");
}

void TextLabelGuiElement::mousePressEvent(QMouseEvent* event)
{
    mouseLeftPressed(event);
    QLabel::mousePressEvent(event);
}

void TextLabelGuiElement::mouseMoveEvent(QMouseEvent* event)
{
    mouseMovedOverItem(event);
    QLabel::mouseMoveEvent(event);
}

void TextLabelGuiElement::mouseReleaseEvent(QMouseEvent* event)
{
    mouseLeftReleased(event);
    QLabel::mouseReleaseEvent(event);
}

void TextLabelGuiElement::keyPressEvent(QKeyEvent* event)
{
    keyPressedCallback(event);
    keyPressed(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QLabel::keyPressEvent(event);
}

void TextLabelGuiElement::keyReleaseEvent(QKeyEvent* event)
{
    keyReleasedCallback(event);
    keyReleased(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QLabel::keyReleaseEvent(event);
}

void TextLabelGuiElement::enterEvent(QEnterEvent* event)
{
    mouseEnteredElement(event);
    QLabel::enterEvent(event);
}

void TextLabelGuiElement::leaveEvent(QEvent* event)
{
    mouseLeftElement(event);
    QLabel::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// EditableTextGuiElement
// ---------------------------------------------------------------------------

EditableTextGuiElement::EditableTextGuiElement(
    QWidget* parent,
    const std::shared_ptr<ElementSettings>& element_settings,
    const GuiCallbacks& callbacks,
    const QPoint& pos,
    const QSize& size)
    : QLineEdit(parent),
      GuiElement(element_settings, callbacks),
      // wx leaves this uninitialized; not preserving that (it's undefined
      // behavior, not a behavior to mimic).
      enter_pressed_(false)
{
    text_ = std::dynamic_pointer_cast<EditableTextSettings>(element_settings)->init_value;
    setText(QString::fromStdString(text_));

    move(pos);
    resize(size);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(this, &QLineEdit::textChanged, this, &EditableTextGuiElement::textChanged);
    connect(this, &QLineEdit::returnPressed, this, &EditableTextGuiElement::returnPressedSlot);
}

void EditableTextGuiElement::textChanged(const QString& new_text)
{
    enter_pressed_ = false;
    text_ = new_text.toStdString();
    sendGuiData();
}

void EditableTextGuiElement::returnPressedSlot()
{
    enter_pressed_ = true;
    sendGuiData();
    enter_pressed_ = false;
}

void EditableTextGuiElement::mousePressEvent(QMouseEvent* event)
{
    mouseLeftPressed(event);
    QLineEdit::mousePressEvent(event);
}

void EditableTextGuiElement::mouseMoveEvent(QMouseEvent* event)
{
    mouseMovedOverItem(event);
    QLineEdit::mouseMoveEvent(event);
}

void EditableTextGuiElement::mouseReleaseEvent(QMouseEvent* event)
{
    mouseLeftReleased(event);
    QLineEdit::mouseReleaseEvent(event);
}

void EditableTextGuiElement::enterEvent(QEnterEvent* event)
{
    mouseEnteredElement(event);
    QLineEdit::enterEvent(event);
}

void EditableTextGuiElement::leaveEvent(QEvent* event)
{
    mouseLeftElement(event);
    QLineEdit::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// DropdownMenuGuiElement
// ---------------------------------------------------------------------------

DropdownMenuGuiElement::DropdownMenuGuiElement(
    QWidget* parent,
    const std::shared_ptr<ElementSettings>& element_settings,
    const GuiCallbacks& callbacks,
    const QPoint& pos,
    const QSize& size)
    : QComboBox(parent),
      GuiElement(element_settings, callbacks)
{
    // wxCB_READONLY equivalent: user picks from the list, cannot type a
    // custom value. QComboBox is non-editable by default.
    setEditable(false);

    elements_ = std::dynamic_pointer_cast<DropdownMenuSettings>(element_settings)->elements;
    for (const std::string& element : elements_)
    {
        addItem(QString::fromStdString(element));
    }

    // wx quirk, preserved: always selects index 0, never consults
    // DropdownMenuSettings::initially_selected_item — see the class
    // comment in gui_elements.h.
    if (!elements_.empty())
    {
        setCurrentIndex(0);
        selected_element_ = elements_.at(0);
    }

    move(pos);
    resize(size);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DropdownMenuGuiElement::selectionChanged);
}

void DropdownMenuGuiElement::selectionChanged(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < elements_.size())
    {
        selected_element_ = elements_.at(index);
    }
    else
    {
        selected_element_ = "";
    }
    sendGuiData();
}

void DropdownMenuGuiElement::mousePressEvent(QMouseEvent* event)
{
    mouseLeftPressed(event);
    QComboBox::mousePressEvent(event);
}

void DropdownMenuGuiElement::mouseMoveEvent(QMouseEvent* event)
{
    mouseMovedOverItem(event);
    QComboBox::mouseMoveEvent(event);
}

void DropdownMenuGuiElement::mouseReleaseEvent(QMouseEvent* event)
{
    mouseLeftReleased(event);
    QComboBox::mouseReleaseEvent(event);
}

void DropdownMenuGuiElement::keyPressEvent(QKeyEvent* event)
{
    keyPressedCallback(event);
    keyPressed(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QComboBox::keyPressEvent(event);
}

void DropdownMenuGuiElement::keyReleaseEvent(QKeyEvent* event)
{
    keyReleasedCallback(event);
    keyReleased(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QComboBox::keyReleaseEvent(event);
}

void DropdownMenuGuiElement::enterEvent(QEnterEvent* event)
{
    mouseEnteredElement(event);
    QComboBox::enterEvent(event);
}

void DropdownMenuGuiElement::leaveEvent(QEvent* event)
{
    mouseLeftElement(event);
    QComboBox::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// ListBoxGuiElement
// ---------------------------------------------------------------------------

ListBoxGuiElement::ListBoxGuiElement(
    QWidget* parent,
    const std::shared_ptr<ElementSettings>& element_settings,
    const GuiCallbacks& callbacks,
    const QPoint& pos,
    const QSize& size)
    : QListWidget(parent),
      GuiElement(element_settings, callbacks)
{
    setSelectionMode(QAbstractItemView::SingleSelection);

    elements_ = std::dynamic_pointer_cast<ListBoxSettings>(element_settings)->elements;
    for (const std::string& element : elements_)
    {
        addItem(QString::fromStdString(element));
    }

    if (!elements_.empty())
    {
        setCurrentRow(0);
        selected_element_ = elements_.at(0);
    }

    move(pos);
    resize(size);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(this, &QListWidget::currentRowChanged, this, [this](int) { selectionChanged(); });
}

void ListBoxGuiElement::selectionChanged()
{
    const int selection = currentRow();
    if (selection >= 0 && static_cast<size_t>(selection) < elements_.size())
    {
        selected_element_ = elements_.at(selection);
    }
    else
    {
        selected_element_ = "";
    }
    sendGuiData();
}

void ListBoxGuiElement::mousePressEvent(QMouseEvent* event)
{
    mouseLeftPressed(event);
    QListWidget::mousePressEvent(event);
}

void ListBoxGuiElement::mouseMoveEvent(QMouseEvent* event)
{
    mouseMovedOverItem(event);
    QListWidget::mouseMoveEvent(event);
}

void ListBoxGuiElement::mouseReleaseEvent(QMouseEvent* event)
{
    mouseLeftReleased(event);
    QListWidget::mouseReleaseEvent(event);
}

void ListBoxGuiElement::keyPressEvent(QKeyEvent* event)
{
    keyPressedCallback(event);
    keyPressed(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QListWidget::keyPressEvent(event);
}

void ListBoxGuiElement::keyReleaseEvent(QKeyEvent* event)
{
    keyReleasedCallback(event);
    keyReleased(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QListWidget::keyReleaseEvent(event);
}

void ListBoxGuiElement::enterEvent(QEnterEvent* event)
{
    mouseEnteredElement(event);
    QListWidget::enterEvent(event);
}

void ListBoxGuiElement::leaveEvent(QEvent* event)
{
    mouseLeftElement(event);
    QListWidget::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// RadioButtonGroupGuiElement
// ---------------------------------------------------------------------------

RadioButtonGroupGuiElement::RadioButtonGroupGuiElement(
    QWidget* parent,
    const std::shared_ptr<ElementSettings>& element_settings,
    const GuiCallbacks& callbacks,
    const QPoint& pos,
    const QSize& size)
    : QGroupBox(QString::fromStdString(std::dynamic_pointer_cast<RadioButtonGroupSettings>(element_settings)->label),
               parent),
      GuiElement(element_settings, callbacks),
      selected_idx_(0)
{
    const auto radio_group_settings = std::dynamic_pointer_cast<RadioButtonGroupSettings>(element_settings);

    QVBoxLayout* layout = new QVBoxLayout(this);
    button_group_ = new QButtonGroup(this);

    int idx = 0;
    for (const RadioButtonSettings& radio_button : radio_group_settings->radio_buttons)
    {
        QRadioButton* btn = new QRadioButton(QString::fromStdString(radio_button.label), this);
        layout->addWidget(btn);
        button_group_->addButton(btn, idx);
        buttons_.push_back(radio_button.label);
        if (idx == 0)
        {
            btn->setChecked(true);
        }
        idx++;
    }

    move(pos);
    resize(size);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(button_group_, &QButtonGroup::idToggled, this, &RadioButtonGroupGuiElement::buttonToggled);
}

void RadioButtonGroupGuiElement::buttonToggled(int id, bool checked)
{
    if (!checked)
    {
        return;
    }
    selected_idx_ = id;
    sendGuiData();
}

void RadioButtonGroupGuiElement::mousePressEvent(QMouseEvent* event)
{
    mouseLeftPressed(event);
    QGroupBox::mousePressEvent(event);
}

void RadioButtonGroupGuiElement::mouseMoveEvent(QMouseEvent* event)
{
    mouseMovedOverItem(event);
    QGroupBox::mouseMoveEvent(event);
}

void RadioButtonGroupGuiElement::mouseReleaseEvent(QMouseEvent* event)
{
    mouseLeftReleased(event);
    QGroupBox::mouseReleaseEvent(event);
}

void RadioButtonGroupGuiElement::keyPressEvent(QKeyEvent* event)
{
    keyPressedCallback(event);
    keyPressed(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QGroupBox::keyPressEvent(event);
}

void RadioButtonGroupGuiElement::keyReleaseEvent(QKeyEvent* event)
{
    keyReleasedCallback(event);
    keyReleased(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QGroupBox::keyReleaseEvent(event);
}

void RadioButtonGroupGuiElement::enterEvent(QEnterEvent* event)
{
    mouseEnteredElement(event);
    QGroupBox::enterEvent(event);
}

void RadioButtonGroupGuiElement::leaveEvent(QEvent* event)
{
    mouseLeftElement(event);
    QGroupBox::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// SliderGuiElement
// ---------------------------------------------------------------------------

SliderGuiElement::SliderGuiElement(
    QWidget* parent,
    const std::shared_ptr<ElementSettings>& element_settings,
    const GuiCallbacks& callbacks,
    const QPoint& pos,
    const QSize& size)
    // wx quirk, preserved: the wx original always constructs its wxSlider
    // with a hardcoded horizontal style regardless of is_horizontal, even
    // though sizing/positioning below fully supports vertical — see the
    // class comment in gui_elements.h. Matched here with Qt::Horizontal
    // always.
    : QSlider(Qt::Horizontal, parent),
      GuiElement(element_settings, callbacks)
{
    const auto slider_settings = std::dynamic_pointer_cast<SliderSettings>(element_settings);

    setMinimum(slider_settings->min_value);
    setMaximum(slider_settings->max_value);
    setValue(slider_settings->init_value);
    slider_value_ = slider_settings->init_value;
    is_horizontal_ = slider_settings->is_horizontal;
    publish_to_local_ = slider_settings->publish_to_local;
    publish_to_serial_ = slider_settings->publish_to_serial;
    id_ = slider_settings->id;

    value_text_ = new QLabel(QString::number(slider_value_), parent);
    min_text_ = new QLabel(QString::number(slider_settings->min_value), parent);
    max_text_ = new QLabel(QString::number(slider_settings->max_value), parent);

    if (is_horizontal_)
    {
        min_y_size_ = 30;
        max_y_size_ = 30;
    }
    else
    {
        min_x_size_ = 30;
        max_x_size_ = 30;
    }

    move(pos);
    resize(size);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    connect(this, &QSlider::valueChanged, this, &SliderGuiElement::sliderValueChanged);
}

void SliderGuiElement::sliderValueChanged(int new_value)
{
    if (new_value == slider_value_)
    {
        return;
    }

    value_text_->setText(QString::number(new_value));
    slider_value_ = new_value;

    if (publish_to_local_)
    {
        sendGuiData();
    }
}

void SliderGuiElement::setElementPositionAndSize()
{
    GuiElement::setElementPositionAndSize();

    const QSize sz = this->size();
    const QPoint p = this->pos();

    if (is_horizontal_)
    {
        value_text_->move(p.x() + sz.width() / 2, p.y() + sz.height() / 2 + 7);
        min_text_->move(p.x() - 13, p.y() + sz.height() / 2 - 10);
        max_text_->move(p.x() + sz.width() + 5, p.y() + sz.height() / 2 - 10);
    }
    else
    {
        value_text_->move(p.x() + sz.width() / 2 + 7, p.y() + sz.height() / 2);
        min_text_->move(p.x() + sz.width() / 2, p.y() - 13);
        max_text_->move(p.x() + sz.width() / 2, p.y() + sz.height() + 5);
    }
}

void SliderGuiElement::updateElementSettings(const std::map<std::string, std::string>& new_settings)
{
    SliderSettings* slider_settings = dynamic_cast<SliderSettings*>(element_settings_.get());

    if (new_settings.count("min_value") > 0U)
    {
        slider_settings->min_value = std::stoi(new_settings.at("min_value"));
        setMinimum(slider_settings->min_value);
    }
    if (new_settings.count("max_value") > 0U)
    {
        slider_settings->max_value = std::stoi(new_settings.at("max_value"));
        setMaximum(slider_settings->max_value);
    }
    min_text_->setText(QString::number(slider_settings->min_value));
    max_text_->setText(QString::number(slider_settings->max_value));

    element_settings_->handle_string = new_settings.at("handle_string");
}

void SliderGuiElement::mousePressEvent(QMouseEvent* event)
{
    mouseLeftPressed(event);
    QSlider::mousePressEvent(event);
}

void SliderGuiElement::mouseMoveEvent(QMouseEvent* event)
{
    mouseMovedOverItem(event);
    QSlider::mouseMoveEvent(event);
}

void SliderGuiElement::mouseReleaseEvent(QMouseEvent* event)
{
    mouseLeftReleased(event);
    QSlider::mouseReleaseEvent(event);
}

void SliderGuiElement::keyPressEvent(QKeyEvent* event)
{
    keyPressedCallback(event);
    keyPressed(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QSlider::keyPressEvent(event);
}

void SliderGuiElement::keyReleaseEvent(QKeyEvent* event)
{
    keyReleasedCallback(event);
    keyReleased(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
    QSlider::keyReleaseEvent(event);
}

void SliderGuiElement::enterEvent(QEnterEvent* event)
{
    mouseEnteredElement(event);
    QSlider::enterEvent(event);
}

void SliderGuiElement::leaveEvent(QEvent* event)
{
    mouseLeftElement(event);
    QSlider::leaveEvent(event);
}
