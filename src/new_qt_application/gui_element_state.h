#ifndef GUI_ELEMENT_STATE_H
#define GUI_ELEMENT_STATE_H

#include "buffered_writer.h"
#include "lumos/plotting/enumerations.h"
#include "lumos/plotting/fillable_uint8_array.h"
#include "project_state/project_settings.h"

class GuiElementState
{
private:
    lumos::GuiElementType type_;
    std::string handle_string_;

public:
    GuiElementState() : type_{lumos::GuiElementType::Unknown}, handle_string_{""} {}

    GuiElementState(const lumos::GuiElementType type, const std::string& handle_string)
        : type_{type}, handle_string_{handle_string}
    {
    }
    virtual ~GuiElementState() {}

    virtual std::uint64_t getTotalNumBytes() const
    {
        return handle_string_.length() + sizeof(std::uint8_t) * 2U + sizeof(std::uint32_t);
    }

    virtual void serializeToBuffer(FillableUInt8Array& output_array) const
    {
        output_array.fillWithStaticType(static_cast<std::uint8_t>(type_));
        output_array.fillWithStaticType(static_cast<std::uint8_t>(handle_string_.length()));
        output_array.fillWithDataFromPointer(handle_string_.data(), handle_string_.length());
    }

    virtual uint16_t serializeToSerialBuffer(BufferedWriter& buffered_writer) const
    {
        return 0;
        // ...
    }

    std::string getHandleString() const
    {
        return handle_string_;
    }

    virtual bool isEqual(const std::shared_ptr<GuiElementState>& other) const
    {
        return other != nullptr && type_ == other->getType() && handle_string_ == other->getHandleString();
    }

    lumos::GuiElementType getType() const
    {
        return type_;
    }
};

class CheckboxState : public GuiElementState
{
private:
    bool is_checked_;

public:
    CheckboxState() = delete;
    CheckboxState(const std::string& handle_string, const bool is_checked)
        : GuiElementState(lumos::GuiElementType::Checkbox, handle_string), is_checked_{is_checked}
    {
    }
    ~CheckboxState() override {}

    std::uint64_t getTotalNumBytes() const override
    {
        return GuiElementState::getTotalNumBytes() + sizeof(std::uint8_t);
    }

    void serializeToBuffer(FillableUInt8Array& output_array) const override
    {
        GuiElementState::serializeToBuffer(output_array);
        output_array.fillWithStaticType(static_cast<std::uint32_t>(1U));  // Payload size
        output_array.fillWithStaticType(static_cast<std::uint8_t>(is_checked_));
    }

    virtual bool isEqual(const std::shared_ptr<GuiElementState>& other) const override
    {
        if (!GuiElementState::isEqual(other))
        {
            return false;
        }
        else
        {
            std::shared_ptr<CheckboxState> other_checkbox = std::dynamic_pointer_cast<CheckboxState>(other);
            if (other_checkbox == nullptr)
            {
                return false;
            }
            return is_checked_ == other_checkbox->is_checked_;
        }
    }
};

class SliderState : public GuiElementState
{
private:
    std::int32_t min_value_;
    std::int32_t max_value_;
    std::int32_t step_size_;
    std::int32_t value_;
    bool is_horizontal_;

public:
    SliderState() = delete;
    SliderState(const std::string& handle_string,
                const std::int32_t min_value,
                const std::int32_t max_value,
                const std::int32_t step_size,
                const std::int32_t value,
                const bool is_horizontal)
        : GuiElementState(lumos::GuiElementType::Slider, handle_string),
          min_value_{min_value},
          max_value_{max_value},
          step_size_{step_size},
          value_{value},
          is_horizontal_{is_horizontal}
    {
    }
    ~SliderState() override {}

    std::uint64_t getTotalNumBytes() const override
    {
        return GuiElementState::getTotalNumBytes() + sizeof(std::int32_t) * 4U + sizeof(std::uint8_t);
    }

    void serializeToBuffer(FillableUInt8Array& output_array) const override
    {
        GuiElementState::serializeToBuffer(output_array);

        output_array.fillWithStaticType(
            static_cast<std::uint32_t>(sizeof(std::int32_t) * 4U + sizeof(std::uint8_t)));  // Payload size

        output_array.fillWithStaticType(static_cast<std::int32_t>(min_value_));
        output_array.fillWithStaticType(static_cast<std::int32_t>(max_value_));
        output_array.fillWithStaticType(static_cast<std::int32_t>(step_size_));
        output_array.fillWithStaticType(static_cast<std::int32_t>(value_));
        output_array.fillWithStaticType(static_cast<std::uint8_t>(is_horizontal_));
    }

    uint16_t serializeToSerialBuffer(BufferedWriter& buffered_writer) const override
    {
        buffered_writer.write(value_);
        return sizeof(int32_t);
    }

    virtual bool isEqual(const std::shared_ptr<GuiElementState>& other) const override
    {
        if (!GuiElementState::isEqual(other))
        {
            return false;
        }
        else
        {
            std::shared_ptr<SliderState> other_slider = std::dynamic_pointer_cast<SliderState>(other);
            if (other_slider == nullptr)
            {
                return false;
            }
            return value_ == other_slider->value_;
        }
    }

    std::int32_t getValue() const
    {
        return value_;
    }
};

class ButtonState : public GuiElementState
{
private:
    bool is_pressed_;

public:
    ButtonState() = delete;
    ButtonState(const std::string& handle_string, const bool is_pressed)
        : GuiElementState(lumos::GuiElementType::Button, handle_string), is_pressed_{is_pressed}
    {
    }
    ~ButtonState() override {}

    std::uint64_t getTotalNumBytes() const override
    {
        return GuiElementState::getTotalNumBytes() + sizeof(std::uint8_t);
    }

    void serializeToBuffer(FillableUInt8Array& output_array) const override
    {
        GuiElementState::serializeToBuffer(output_array);

        output_array.fillWithStaticType(static_cast<std::uint32_t>(1U));  // Payload size
        output_array.fillWithStaticType(static_cast<std::uint8_t>(is_pressed_));
    }

    uint16_t serializeToSerialBuffer(BufferedWriter& buffered_writer) const override
    {
        const uint16_t is_pressed = static_cast<uint16_t>(is_pressed_);
        buffered_writer.write(is_pressed);
        return sizeof(uint16_t);
    }

    bool isPressed() const
    {
        return is_pressed_;
    }

    virtual bool isEqual(const std::shared_ptr<GuiElementState>& other) const override
    {
        if (!GuiElementState::isEqual(other))
        {
            return false;
        }
        else
        {
            std::shared_ptr<ButtonState> other_button = std::dynamic_pointer_cast<ButtonState>(other);
            if (other_button == nullptr)
            {
                return false;
            }
            return is_pressed_ == other_button->is_pressed_;
        }
    }
};

class TextLabelState : public GuiElementState
{
private:
    std::string label_;

public:
    TextLabelState() = delete;
    TextLabelState(const std::string& handle_string, const std::string& label)
        : GuiElementState(lumos::GuiElementType::TextLabel, handle_string), label_{label}
    {
    }
    ~TextLabelState() override {}

    std::uint64_t getTotalNumBytes() const override
    {
        return GuiElementState::getTotalNumBytes() + sizeof(std::uint8_t) + label_.length();
    }

    void serializeToBuffer(FillableUInt8Array& output_array) const override
    {
        GuiElementState::serializeToBuffer(output_array);

        output_array.fillWithStaticType(static_cast<std::uint32_t>(label_.length() + 1U));  // Payload size
        output_array.fillWithStaticType(static_cast<std::uint8_t>(label_.length()));
        output_array.fillWithDataFromPointer(label_.data(), label_.length());
    }

    virtual bool isEqual(const std::shared_ptr<GuiElementState>& other) const override
    {
        if (!GuiElementState::isEqual(other))
        {
            return false;
        }
        else
        {
            std::shared_ptr<TextLabelState> other_text_label = std::dynamic_pointer_cast<TextLabelState>(other);
            if (other_text_label == nullptr)
            {
                return false;
            }
            return label_ == other_text_label->label_;
        }
    }
};

class ListBoxState : public GuiElementState
{
private:
    std::vector<std::string> elements_;
    std::string selected_element_;

public:
    ListBoxState() = delete;
    ListBoxState(const std::string& handle_string,
                 const std::vector<std::string>& elements,
                 const std::string& selected_element)
        : GuiElementState{lumos::GuiElementType::ListBox, handle_string},
          elements_{elements},
          selected_element_{selected_element}
    {
    }
    ~ListBoxState() override {}

    std::uint64_t getTotalNumBytes() const override
    {
        std::uint64_t total_num_bytes = GuiElementState::getTotalNumBytes();
        total_num_bytes += sizeof(std::uint8_t) + selected_element_.length();

        total_num_bytes += sizeof(std::uint16_t);  // Number of elements

        for (const auto& element : elements_)
        {
            total_num_bytes += sizeof(std::uint8_t) + element.length();
        }
        return total_num_bytes;
    }

    void serializeToBuffer(FillableUInt8Array& output_array) const override
    {
        GuiElementState::serializeToBuffer(output_array);

        std::uint32_t payload_size = sizeof(std::uint8_t) + selected_element_.length();

        payload_size += sizeof(std::uint16_t);  // Number of elements

        for (const auto& element : elements_)
        {
            payload_size += sizeof(std::uint8_t) + element.length();
        }

        output_array.fillWithStaticType(payload_size);
        output_array.fillWithStaticType(static_cast<std::uint8_t>(selected_element_.length()));
        output_array.fillWithDataFromPointer(selected_element_.data(), selected_element_.length());

        output_array.fillWithStaticType(static_cast<std::uint16_t>(elements_.size()));

        for (const auto& element : elements_)
        {
            output_array.fillWithStaticType(static_cast<std::uint8_t>(element.length()));
            output_array.fillWithDataFromPointer(element.data(), element.length());
        }
    }

    virtual bool isEqual(const std::shared_ptr<GuiElementState>& other) const override
    {
        if (!GuiElementState::isEqual(other))
        {
            return false;
        }
        else
        {
            std::shared_ptr<ListBoxState> other_list_box = std::dynamic_pointer_cast<ListBoxState>(other);
            if (other_list_box == nullptr)
            {
                return false;
            }
            return elements_ == other_list_box->elements_ && selected_element_ == other_list_box->selected_element_;
        }
    }
};

class EditableTextState : public GuiElementState
{
private:
    bool enter_pressed_;
    std::string text_;

public:
    EditableTextState() = delete;
    EditableTextState(const std::string& handle_string, const bool enter_pressed, const std::string& text)
        : GuiElementState{lumos::GuiElementType::EditableText, handle_string},
          enter_pressed_{enter_pressed},
          text_{text}
    {
    }
    ~EditableTextState() override {}

    std::uint64_t getTotalNumBytes() const override
    {
        std::uint64_t total_num_bytes = GuiElementState::getTotalNumBytes();
        total_num_bytes += sizeof(std::uint8_t) + sizeof(std::uint8_t) + text_.length();

        return total_num_bytes;
    }

    void serializeToBuffer(FillableUInt8Array& output_array) const override
    {
        GuiElementState::serializeToBuffer(output_array);

        const std::uint32_t payload_size = sizeof(std::uint8_t) + sizeof(std::uint8_t) + text_.length();

        output_array.fillWithStaticType(payload_size);
        output_array.fillWithStaticType(static_cast<std::uint8_t>(enter_pressed_));
        output_array.fillWithStaticType(static_cast<std::uint8_t>(text_.length()));
        output_array.fillWithDataFromPointer(text_.data(), text_.length());
    }

    virtual bool isEqual(const std::shared_ptr<GuiElementState>& other) const override
    {
        if (!GuiElementState::isEqual(other))
        {
            return false;
        }
        else
        {
            std::shared_ptr<EditableTextState> other_editable_text =
                std::dynamic_pointer_cast<EditableTextState>(other);
            if (other_editable_text == nullptr)
            {
                return false;
            }
            return text_ == other_editable_text->text_;
        }
    }
};

class DropdownMenuState : public GuiElementState
{
private:
    std::vector<std::string> elements_;
    std::string selected_element_;

public:
    DropdownMenuState() = delete;
    DropdownMenuState(const std::string& handle_string,
                      const std::vector<std::string>& elements,
                      const std::string& selected_element)
        : GuiElementState{lumos::GuiElementType::DropdownMenu, handle_string},
          elements_{elements},
          selected_element_{selected_element}
    {
    }
    ~DropdownMenuState() override {}

    std::uint64_t getTotalNumBytes() const override
    {
        std::uint64_t total_num_bytes = GuiElementState::getTotalNumBytes();
        total_num_bytes += sizeof(std::uint8_t) + selected_element_.length();

        total_num_bytes += sizeof(std::uint16_t);  // Number of elements

        for (const auto& element : elements_)
        {
            total_num_bytes += sizeof(std::uint8_t) + element.length();
        }
        return total_num_bytes;
    }

    void serializeToBuffer(FillableUInt8Array& output_array) const override
    {
        GuiElementState::serializeToBuffer(output_array);

        std::uint32_t payload_size = sizeof(std::uint8_t) + selected_element_.length();

        payload_size += sizeof(std::uint16_t);  // Number of elements

        for (const auto& element : elements_)
        {
            payload_size += sizeof(std::uint8_t) + element.length();
        }

        output_array.fillWithStaticType(payload_size);
        output_array.fillWithStaticType(static_cast<std::uint8_t>(selected_element_.length()));
        output_array.fillWithDataFromPointer(selected_element_.data(), selected_element_.length());

        output_array.fillWithStaticType(static_cast<std::uint16_t>(elements_.size()));

        for (const auto& element : elements_)
        {
            output_array.fillWithStaticType(static_cast<std::uint8_t>(element.length()));
            output_array.fillWithDataFromPointer(element.data(), element.length());
        }
    }

    virtual bool isEqual(const std::shared_ptr<GuiElementState>& other) const override
    {
        if (!GuiElementState::isEqual(other))
        {
            return false;
        }
        else
        {
            std::shared_ptr<DropdownMenuState> other_dropdown_menu =
                std::dynamic_pointer_cast<DropdownMenuState>(other);
            if (other_dropdown_menu == nullptr)
            {
                return false;
            }
            return selected_element_ == other_dropdown_menu->selected_element_;
        }
    }
};

class RadioButtonGroupState : public GuiElementState
{
private:
    std::vector<std::string> buttons_;
    std::int32_t selected_button_;

public:
    RadioButtonGroupState() = delete;
    RadioButtonGroupState(const std::string& handle_string,
                          const std::vector<std::string>& buttons,
                          const std::int32_t selected_button)
        : GuiElementState{lumos::GuiElementType::RadioButtonGroup, handle_string},
          buttons_{buttons},
          selected_button_{selected_button}
    {
    }
    ~RadioButtonGroupState() override {}

    std::uint64_t getTotalNumBytes() const override
    {
        std::uint64_t total_num_bytes = GuiElementState::getTotalNumBytes();

        total_num_bytes += sizeof(std::uint16_t);  // Number of buttons
        total_num_bytes += sizeof(std::uint32_t);  // Selected button

        for (const auto& button : buttons_)
        {
            total_num_bytes += sizeof(std::uint8_t) + button.length();
        }

        return total_num_bytes;
    }

    void serializeToBuffer(FillableUInt8Array& output_array) const override
    {
        GuiElementState::serializeToBuffer(output_array);

        std::uint32_t payload_size = 0U;

        payload_size += sizeof(std::uint16_t);  // Number of buttons
        payload_size += sizeof(std::uint32_t);  // Selected button

        for (const auto& button : buttons_)
        {
            payload_size += sizeof(std::uint8_t) + button.length();
        }

        output_array.fillWithStaticType(payload_size);
        output_array.fillWithStaticType(selected_button_);

        output_array.fillWithStaticType(static_cast<std::uint16_t>(buttons_.size()));

        for (const auto& button : buttons_)
        {
            output_array.fillWithStaticType(static_cast<std::uint8_t>(button.length()));
            output_array.fillWithDataFromPointer(button.data(), button.length());
        }
    }

    virtual bool isEqual(const std::shared_ptr<GuiElementState>& other) const override
    {
        if (!GuiElementState::isEqual(other))
        {
            return false;
        }
        else
        {
            std::shared_ptr<RadioButtonGroupState> other_radio_button_group =
                std::dynamic_pointer_cast<RadioButtonGroupState>(other);
            if (other_radio_button_group == nullptr)
            {
                return false;
            }
            return selected_button_ == other_radio_button_group->selected_button_;
        }
    }
};

#endif  // GUI_ELEMENT_STATE_H
