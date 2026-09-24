#ifndef NEW_QT_APPLICATION_GUI_ELEMENTS_H_
#define NEW_QT_APPLICATION_GUI_ELEMENTS_H_

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QVBoxLayout>

#include <functional>
#include <memory>
#include <vector>

#include "communication/received_data.h"
#include "gui_callbacks.h"
#include "gui_element.h"
#include "lumos/math.h"
#include "lumos/plotting/enumerations.h"
#include "project_state/project_settings.h"

// Adapted from main_application/gui_elements.{h,cpp}. Each class follows
// the same pattern as PlotPane: multiple inheritance from a native Qt
// widget plus GuiElement, with mouse/keyboard events routed through
// GuiElement's edit-mode dispatch first (see plot_pane.cpp's comment on
// mousePressEvent/mouseMoveEvent/mouseReleaseEvent for why).
//
// A handful of wx behaviors were found, while porting, to be quirks or
// latent bugs rather than deliberate design (see the research note this was
// built from, summarized in CURRENT_STATE.md) — e.g. Checkbox ignores
// publish_to_local and always sends; DropdownMenu ignores
// initially_selected_item and always starts at index 0; Slider is
// constructed with a hardcoded horizontal wx style regardless of its
// is_horizontal setting even though its sizing/positioning code fully
// supports vertical orientation. None of these were reported as things the
// user wants changed, so each is preserved exactly as wx behaves, with a
// comment at the specific line — flagged rather than silently fixed or
// silently kept, per the "ask before deciding" rule for anything not
// already settled.
class ButtonGuiElement : public QPushButton, public GuiElement
{
    Q_OBJECT

private:
    bool is_pressed_;
    bool publish_to_local_;
    bool publish_to_serial_;

    void buttonClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

public:
    ButtonGuiElement(QWidget* parent,
                     const std::shared_ptr<ElementSettings>& element_settings,
                     const GuiCallbacks& callbacks,
                     const QPoint& pos,
                     const QSize& size);

    void updateElementSettings(const std::map<std::string, std::string>& new_settings) override;
    void setLabel(const std::string& new_label) override;

    void keyPressedElementSpecific(const char /*key*/) override {}
    void keyReleasedElementSpecific(const char /*key*/) override {}

    void mouseLeftPressedGuiElementSpecific(QMouseEvent* /*event*/) override
    {
        is_pressed_ = true;
    }

    void mouseLeftReleasedGuiElementSpecific(QMouseEvent* /*event*/) override
    {
        is_pressed_ = false;
    }

    std::uint64_t getGuiPayloadSize() const override
    {
        return sizeof(std::int8_t);
    }

    void fillGuiPayload(FillableUInt8Array& output_array) const override
    {
        output_array.fillWithStaticType(static_cast<std::uint8_t>(is_pressed_));
    }

    std::shared_ptr<GuiElementState> getGuiElementState() const override
    {
        return std::make_shared<ButtonState>(element_settings_->handle_string, is_pressed_);
    }

    void show() override
    {
        QPushButton::show();
    }

    void hide() override
    {
        QPushButton::hide();
    }

    QWidget* getParentWidget() const override
    {
        return this->parentWidget();
    }

    QPoint getPosition() const override
    {
        return this->pos();
    }

    QSize getSize() const override
    {
        return this->size();
    }

    void setPosition(const QPoint& new_pos) override
    {
        this->move(new_pos);
    }

    void setSize(const QSize& new_size) override
    {
        this->resize(new_size);
    }
};

// ---------------------------------------------------------------------------

class CheckboxGuiElement : public QCheckBox, public GuiElement
{
    Q_OBJECT

private:
    void checkboxToggled();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

public:
    CheckboxGuiElement(QWidget* parent,
                       const std::shared_ptr<ElementSettings>& element_settings,
                       const GuiCallbacks& callbacks,
                       const QPoint& pos,
                       const QSize& size);

    // wx quirk, preserved: publish_to_local/publish_to_serial are never
    // consulted here — a checkbox always calls sendGuiData() unconditionally
    // (main_application/gui_elements.cpp's checkBoxCallback).
    void updateElementSettings(const std::map<std::string, std::string>& /*new_settings*/) override {}

    void keyPressedElementSpecific(const char /*key*/) override {}
    void keyReleasedElementSpecific(const char /*key*/) override {}

    std::uint64_t getGuiPayloadSize() const override
    {
        return sizeof(std::int8_t);
    }

    void fillGuiPayload(FillableUInt8Array& output_array) const override
    {
        output_array.fillWithStaticType(static_cast<std::uint8_t>(this->isChecked()));
    }

    std::shared_ptr<GuiElementState> getGuiElementState() const override
    {
        return std::make_shared<CheckboxState>(element_settings_->handle_string, this->isChecked());
    }

    void show() override
    {
        QCheckBox::show();
    }

    void hide() override
    {
        QCheckBox::hide();
    }

    QWidget* getParentWidget() const override
    {
        return this->parentWidget();
    }

    QPoint getPosition() const override
    {
        return this->pos();
    }

    QSize getSize() const override
    {
        return this->size();
    }

    void setPosition(const QPoint& new_pos) override
    {
        this->move(new_pos);
    }

    void setSize(const QSize& new_size) override
    {
        this->resize(new_size);
    }
};

// ---------------------------------------------------------------------------

class TextLabelGuiElement : public QLabel, public GuiElement
{
    Q_OBJECT

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

public:
    TextLabelGuiElement(QWidget* parent,
                        const std::shared_ptr<ElementSettings>& element_settings,
                        const GuiCallbacks& callbacks,
                        const QPoint& pos,
                        const QSize& size);

    void updateElementSettings(const std::map<std::string, std::string>& new_settings) override;
    void setLabel(const std::string& new_label) override;

    void keyPressedElementSpecific(const char /*key*/) override {}
    void keyReleasedElementSpecific(const char /*key*/) override {}

    // A text label never triggers sendGuiData() (no user interaction to
    // fire it, matching wx — TextLabel has no bound event that calls it),
    // so this payload is never actually sent, but is implemented correctly
    // (from the live text) rather than replicating wx's dead-code bug where
    // it always reports an empty label — see CURRENT_STATE.md.
    std::uint64_t getGuiPayloadSize() const override
    {
        return sizeof(std::int8_t) + text().toStdString().length();
    }

    void fillGuiPayload(FillableUInt8Array& output_array) const override
    {
        const std::string label = text().toStdString();
        output_array.fillWithStaticType(static_cast<std::uint8_t>(label.length()));
        output_array.fillWithDataFromPointer(label.data(), label.length());
    }

    std::shared_ptr<GuiElementState> getGuiElementState() const override
    {
        return std::make_shared<TextLabelState>(element_settings_->handle_string, text().toStdString());
    }

    void show() override
    {
        QLabel::show();
    }

    void hide() override
    {
        QLabel::hide();
    }

    QWidget* getParentWidget() const override
    {
        return this->parentWidget();
    }

    QPoint getPosition() const override
    {
        return this->pos();
    }

    QSize getSize() const override
    {
        return this->size();
    }

    void setPosition(const QPoint& new_pos) override
    {
        this->move(new_pos);
    }

    void setSize(const QSize& new_size) override
    {
        this->resize(new_size);
    }
};

// ---------------------------------------------------------------------------

class EditableTextGuiElement : public QLineEdit, public GuiElement
{
    Q_OBJECT

private:
    std::string text_;
    bool enter_pressed_;

    void textChanged(const QString& new_text);
    void returnPressedSlot();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

public:
    EditableTextGuiElement(QWidget* parent,
                           const std::shared_ptr<ElementSettings>& element_settings,
                           const GuiCallbacks& callbacks,
                           const QPoint& pos,
                           const QSize& size);

    // Fully unimplemented in the wx original too (no keys handled) — not a
    // gap introduced by this port.
    void updateElementSettings(const std::map<std::string, std::string>& /*new_settings*/) override {}

    // Text entry is handled natively by QLineEdit's own key handling, not
    // by GuiElement's edit-mode machinery — matches wx (its own key-editor
    // infrastructure is only for the Ctrl-drag-resize mode).
    void keyPressedElementSpecific(const char /*key*/) override {}
    void keyReleasedElementSpecific(const char /*key*/) override {}

    std::uint64_t getGuiPayloadSize() const override
    {
        return sizeof(std::uint8_t) + sizeof(std::uint8_t) + text_.length();
    }

    void fillGuiPayload(FillableUInt8Array& output_array) const override
    {
        output_array.fillWithStaticType(static_cast<std::uint8_t>(enter_pressed_));
        output_array.fillWithStaticType(static_cast<std::uint8_t>(text_.length()));
        output_array.fillWithDataFromPointer(text_.data(), text_.length());
    }

    std::shared_ptr<GuiElementState> getGuiElementState() const override
    {
        return std::make_shared<EditableTextState>(element_settings_->handle_string, enter_pressed_, text_);
    }

    void show() override
    {
        QLineEdit::show();
    }

    void hide() override
    {
        QLineEdit::hide();
    }

    QWidget* getParentWidget() const override
    {
        return this->parentWidget();
    }

    QPoint getPosition() const override
    {
        return this->pos();
    }

    QSize getSize() const override
    {
        return this->size();
    }

    void setPosition(const QPoint& new_pos) override
    {
        this->move(new_pos);
    }

    void setSize(const QSize& new_size) override
    {
        this->resize(new_size);
    }
};

// ---------------------------------------------------------------------------

class DropdownMenuGuiElement : public QComboBox, public GuiElement
{
    Q_OBJECT

private:
    std::vector<std::string> elements_;
    std::string selected_element_;

    void selectionChanged(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

public:
    DropdownMenuGuiElement(QWidget* parent,
                           const std::shared_ptr<ElementSettings>& element_settings,
                           const GuiCallbacks& callbacks,
                           const QPoint& pos,
                           const QSize& size);

    // Fully unimplemented in the wx original too — not a gap from this port.
    void updateElementSettings(const std::map<std::string, std::string>& /*new_settings*/) override {}

    void keyPressedElementSpecific(const char /*key*/) override {}
    void keyReleasedElementSpecific(const char /*key*/) override {}

    std::uint64_t getGuiPayloadSize() const override
    {
        std::uint64_t total = sizeof(std::uint8_t) + selected_element_.length();
        total += sizeof(std::uint16_t);
        for (const auto& e : elements_)
        {
            total += sizeof(std::uint8_t) + e.length();
        }
        return total;
    }

    void fillGuiPayload(FillableUInt8Array& output_array) const override
    {
        output_array.fillWithStaticType(static_cast<std::uint8_t>(selected_element_.length()));
        output_array.fillWithDataFromPointer(selected_element_.data(), selected_element_.length());
        output_array.fillWithStaticType(static_cast<std::uint16_t>(elements_.size()));
        for (const auto& e : elements_)
        {
            output_array.fillWithStaticType(static_cast<std::uint8_t>(e.length()));
            output_array.fillWithDataFromPointer(e.data(), e.length());
        }
    }

    std::shared_ptr<GuiElementState> getGuiElementState() const override
    {
        return std::make_shared<DropdownMenuState>(element_settings_->handle_string, elements_, selected_element_);
    }

    void show() override
    {
        QComboBox::show();
    }

    void hide() override
    {
        QComboBox::hide();
    }

    QWidget* getParentWidget() const override
    {
        return this->parentWidget();
    }

    QPoint getPosition() const override
    {
        return this->pos();
    }

    QSize getSize() const override
    {
        return this->size();
    }

    void setPosition(const QPoint& new_pos) override
    {
        this->move(new_pos);
    }

    void setSize(const QSize& new_size) override
    {
        this->resize(new_size);
    }
};

// ---------------------------------------------------------------------------

class ListBoxGuiElement : public QListWidget, public GuiElement
{
    Q_OBJECT

private:
    std::vector<std::string> elements_;
    std::string selected_element_;

    void selectionChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

public:
    ListBoxGuiElement(QWidget* parent,
                      const std::shared_ptr<ElementSettings>& element_settings,
                      const GuiCallbacks& callbacks,
                      const QPoint& pos,
                      const QSize& size);

    // Fully unimplemented in the wx original too — not a gap from this port.
    void updateElementSettings(const std::map<std::string, std::string>& /*new_settings*/) override {}

    void keyPressedElementSpecific(const char /*key*/) override {}
    void keyReleasedElementSpecific(const char /*key*/) override {}

    std::uint64_t getGuiPayloadSize() const override
    {
        std::uint64_t total = sizeof(std::uint8_t) + selected_element_.length();
        total += sizeof(std::uint16_t);
        for (const auto& e : elements_)
        {
            total += sizeof(std::uint8_t) + e.length();
        }
        return total;
    }

    void fillGuiPayload(FillableUInt8Array& output_array) const override
    {
        output_array.fillWithStaticType(static_cast<std::uint8_t>(selected_element_.length()));
        output_array.fillWithDataFromPointer(selected_element_.data(), selected_element_.length());
        output_array.fillWithStaticType(static_cast<std::uint16_t>(elements_.size()));
        for (const auto& e : elements_)
        {
            output_array.fillWithStaticType(static_cast<std::uint8_t>(e.length()));
            output_array.fillWithDataFromPointer(e.data(), e.length());
        }
    }

    std::shared_ptr<GuiElementState> getGuiElementState() const override
    {
        return std::make_shared<ListBoxState>(element_settings_->handle_string, elements_, selected_element_);
    }

    void show() override
    {
        QListWidget::show();
    }

    void hide() override
    {
        QListWidget::hide();
    }

    QWidget* getParentWidget() const override
    {
        return this->parentWidget();
    }

    QPoint getPosition() const override
    {
        return this->pos();
    }

    QSize getSize() const override
    {
        return this->size();
    }

    void setPosition(const QPoint& new_pos) override
    {
        this->move(new_pos);
    }

    void setSize(const QSize& new_size) override
    {
        this->resize(new_size);
    }
};

// ---------------------------------------------------------------------------

// wx builds this on wxRadioBox (a single native widget with N built-in
// sub-buttons). Qt has no equivalent single widget, so this wraps a
// QGroupBox containing a QButtonGroup of QRadioButtons in a QVBoxLayout —
// matching wx's wxRA_SPECIFY_COLS with 1 column (buttons stacked
// vertically). The sub-buttons are plain Qt children the group box lays
// out itself; they aren't separately tracked GuiElements (matches wx).
class RadioButtonGroupGuiElement : public QGroupBox, public GuiElement
{
    Q_OBJECT

private:
    std::vector<std::string> buttons_;
    std::int32_t selected_idx_;
    QButtonGroup* button_group_;

    void buttonToggled(int id, bool checked);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

public:
    RadioButtonGroupGuiElement(QWidget* parent,
                              const std::shared_ptr<ElementSettings>& element_settings,
                              const GuiCallbacks& callbacks,
                              const QPoint& pos,
                              const QSize& size);

    // Fully unimplemented in the wx original too — not a gap from this port.
    void updateElementSettings(const std::map<std::string, std::string>& /*new_settings*/) override {}

    void keyPressedElementSpecific(const char /*key*/) override {}
    void keyReleasedElementSpecific(const char /*key*/) override {}

    std::uint64_t getGuiPayloadSize() const override
    {
        std::uint64_t total = 0U;
        total += sizeof(std::uint16_t);
        total += sizeof(std::uint32_t);
        for (const auto& button : buttons_)
        {
            total += sizeof(std::uint8_t) + button.length();
        }
        return total;
    }

    void fillGuiPayload(FillableUInt8Array& output_array) const override
    {
        output_array.fillWithStaticType(selected_idx_);
        output_array.fillWithStaticType(static_cast<std::uint16_t>(buttons_.size()));
        for (const auto& button : buttons_)
        {
            output_array.fillWithStaticType(static_cast<std::uint8_t>(button.length()));
            output_array.fillWithDataFromPointer(button.data(), button.length());
        }
    }

    std::shared_ptr<GuiElementState> getGuiElementState() const override
    {
        return std::make_shared<RadioButtonGroupState>(element_settings_->handle_string, buttons_, selected_idx_);
    }

    void show() override
    {
        QGroupBox::show();
    }

    void hide() override
    {
        QGroupBox::hide();
    }

    QWidget* getParentWidget() const override
    {
        return this->parentWidget();
    }

    QPoint getPosition() const override
    {
        return this->pos();
    }

    QSize getSize() const override
    {
        return this->size();
    }

    void setPosition(const QPoint& new_pos) override
    {
        this->move(new_pos);
    }

    void setSize(const QSize& new_size) override
    {
        this->resize(new_size);
    }
};

// ---------------------------------------------------------------------------

class SliderGuiElement : public QSlider, public GuiElement
{
    Q_OBJECT

private:
    std::int32_t slider_value_;
    QLabel* value_text_;
    QLabel* min_text_;
    QLabel* max_text_;
    bool is_horizontal_;
    bool publish_to_local_;
    bool publish_to_serial_;

    void sliderValueChanged(int new_value);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

public:
    SliderGuiElement(QWidget* parent,
                     const std::shared_ptr<ElementSettings>& element_settings,
                     const GuiCallbacks& callbacks,
                     const QPoint& pos,
                     const QSize& size);

    void updateElementSettings(const std::map<std::string, std::string>& new_settings) override;

    void keyPressedElementSpecific(const char /*key*/) override {}
    void keyReleasedElementSpecific(const char /*key*/) override {}

    void setElementPositionAndSize() override;

    std::uint64_t getGuiPayloadSize() const override
    {
        return 4U * sizeof(std::int32_t);
    }

    void fillGuiPayload(FillableUInt8Array& output_array) const override
    {
        const auto slider_settings = std::dynamic_pointer_cast<SliderSettings>(element_settings_);
        output_array.fillWithStaticType(static_cast<std::int32_t>(slider_settings->min_value));
        output_array.fillWithStaticType(static_cast<std::int32_t>(slider_settings->max_value));
        output_array.fillWithStaticType(static_cast<std::int32_t>(slider_settings->step_size));
        output_array.fillWithStaticType(slider_value_);
    }

    std::shared_ptr<GuiElementState> getGuiElementState() const override
    {
        const auto slider_settings = std::dynamic_pointer_cast<SliderSettings>(element_settings_);
        // wx quirk, preserved: always reports is_horizontal=true here
        // regardless of the slider's actual orientation (a literal `true`
        // in the original, not the is_horizontal_ member) — see
        // CURRENT_STATE.md.
        return std::make_shared<SliderState>(element_settings_->handle_string, slider_settings->min_value,
                                             slider_settings->max_value, slider_settings->step_size, this->value(),
                                             true);
    }

    void show() override
    {
        QSlider::show();
        value_text_->show();
        min_text_->show();
        max_text_->show();
    }

    void hide() override
    {
        QSlider::hide();
        value_text_->hide();
        min_text_->hide();
        max_text_->hide();
    }

    QWidget* getParentWidget() const override
    {
        return this->parentWidget();
    }

    QPoint getPosition() const override
    {
        return this->pos();
    }

    QSize getSize() const override
    {
        return this->size();
    }

    void setPosition(const QPoint& new_pos) override
    {
        this->move(new_pos);
    }

    void setSize(const QSize& new_size) override
    {
        QSize final_size = new_size;
        final_size.setWidth(std::max(min_x_size_, std::min(max_x_size_, final_size.width())));
        final_size.setHeight(std::max(min_y_size_, std::min(max_y_size_, final_size.height())));
        this->resize(final_size);
    }
};

#endif  // NEW_QT_APPLICATION_GUI_ELEMENTS_H_
