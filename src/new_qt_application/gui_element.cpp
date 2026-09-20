#include "gui_element.h"

#include <QApplication>
#include <QGuiApplication>
#include <iostream>
#include <stdexcept>

#include "lumos/plotting/internal.h"

GuiElement::GuiElement(
    const std::shared_ptr<ElementSettings>& element_settings,
    const std::function<void(const char key)>& notify_main_window_key_pressed,
    const std::function<void(const char key)>& notify_main_window_key_released,
    const std::function<void(const QPoint pos, const std::string& elem_name)>&
        notify_parent_window_right_mouse_pressed,
    const std::function<void()>& notify_main_window_about_modification,
    const std::function<void(const QPoint& pos, const QSize& size, const bool is_editing)>& notify_tab_about_editing,
    const std::function<void(const Color_t, const std::string&)>& push_text_to_cmdl_output_window)
    : element_settings_{element_settings},
      notify_main_window_key_pressed_{notify_main_window_key_pressed},
      notify_main_window_key_released_{notify_main_window_key_released},
      notify_parent_window_right_mouse_pressed_{notify_parent_window_right_mouse_pressed},
      notify_main_window_about_modification_{notify_main_window_about_modification},
      notify_tab_about_editing_{notify_tab_about_editing},
      push_text_to_cmdl_output_window_{push_text_to_cmdl_output_window}
{
    control_pressed_at_mouse_press_ = false;
    shift_pressed_at_mouse_press_ = false;
    mouse_is_inside_ = false;

    edit_size_margin_ = 5.0f;

    min_x_size_ = 10;
    max_x_size_ = 10000;
    min_y_size_ = 10;
    max_y_size_ = 10000;
}

void GuiElement::keyPressed(const char key)
{
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
    {
        QWidget* parent_handle = this->getParentWidget();

        const float x0 = static_cast<float>(this->getPosition().x());
        const float y0 = static_cast<float>(this->getPosition().y());

        const float x1 = x0 + static_cast<float>(this->getSize().width());
        const float y1 = y0 + static_cast<float>(this->getSize().height());

        const QPoint pt = QCursor::pos() - parent_handle->mapToGlobal(QPoint(0, 0));

        if ((pt.x() > x0) && (pt.x() < x1) && (pt.y() > y0) && (pt.y() < y1))
        {
            setCursorDependingOnMousePos(pt - this->getPosition());
        }
        if (mouse_is_inside_)
        {
            notify_tab_about_editing_(this->getPosition(), this->getSize(), true);
        }
    }

    keyPressedElementSpecific(key);
}

void GuiElement::keyReleased(const char key)
{
    if (!QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
    {
        if (!(QGuiApplication::mouseButtons() & Qt::LeftButton))
        {
            this->setCursorDependingOnMousePos(this->getPosition());  // placeholder cursor reset below
        }
    }

    keyReleasedElementSpecific(key);
}

void GuiElement::mouseEnteredElement(QEvent* /*event*/)
{
    mouse_is_inside_ = true;
    const QPoint current_element_position = this->getPosition();
    const QPoint current_mouse_local_position = this->getParentWidget()->mapFromGlobal(QCursor::pos()) - current_element_position;
    previous_mouse_pos_ = current_element_position + current_mouse_local_position;

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
    {
        notify_tab_about_editing_(this->getPosition(), this->getSize(), true);
        setCursorDependingOnMousePos(current_mouse_local_position);
    }
}

void GuiElement::mouseLeftElement(QEvent* /*event*/)
{
    mouse_is_inside_ = false;
    notify_tab_about_editing_(QPoint{0, 0}, QSize{0, 0}, false);
}

void GuiElement::mouseLeftPressed(QMouseEvent* event)
{
    const QPoint current_mouse_position = event->pos();

    element_pos_at_press_ = this->getPosition();
    element_size_at_press_ = this->getSize();
    mouse_pos_at_press_ = event->pos() + this->getPosition();
    previous_mouse_pos_ = mouse_pos_at_press_;

    const auto [bnd, bnd_with_margin] = getBounds();

    cursor_state_at_press_ = getCursorSquareState(bnd, bnd_with_margin, current_mouse_position);

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
    {
        control_pressed_at_mouse_press_ = true;
    }
    else
    {
        mouseLeftPressedGuiElementSpecific(event);
    }
}

void GuiElement::mouseLeftReleased(QMouseEvent* event)
{
    if (control_pressed_at_mouse_press_)
    {
        control_pressed_at_mouse_press_ = false;
        if (!QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
        {
            notify_tab_about_editing_(QPoint{0, 0}, QSize{0, 0}, false);
        }
    }
    else
    {
        mouseLeftReleasedGuiElementSpecific(event);
    }
}

void GuiElement::mouseMovedOverItem(QMouseEvent* event)
{
    const QPoint current_mouse_position = event->pos();
    const QPoint current_pane_position = this->getPosition();

    current_mouse_pos_ = current_pane_position + current_mouse_position;

    if (control_pressed_at_mouse_press_ && (event->buttons() & Qt::LeftButton))
    {
        adjustPaneSizeOnMouseMoved();
        notify_tab_about_editing_(this->getPosition(), this->getSize(), true);

        notify_main_window_about_modification_();
    }
    else
    {
        mouseMovedGuiElementSpecific(event);
    }

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) && !(event->buttons() & Qt::LeftButton))
    {
        setCursorDependingOnMousePos(event->pos());
    }

    previous_mouse_pos_ = current_mouse_pos_;
}

void GuiElement::mouseRightPressed(QMouseEvent* event)
{
    const QPoint current_mouse_position_local = event->pos();
    const QPoint current_pane_position = this->getPosition();

    previous_mouse_pos_ = current_pane_position + current_mouse_position_local;

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
    {
        shift_pressed_at_mouse_press_ = true;
    }
    else
    {
        notify_parent_window_right_mouse_pressed_(this->getPosition() + event->pos(),
                                                   element_settings_->handle_string);
    }
}

void GuiElement::mouseRightReleased(QMouseEvent* event)
{
    mouseRightPressedGuiElementSpecific(event);

    shift_pressed_at_mouse_press_ = false;
}

void GuiElement::adjustPaneSizeOnMouseMoved()
{
    const QPoint delta = current_mouse_pos_ - previous_mouse_pos_;
    const QPoint delta_x(delta.x(), 0);
    const QPoint delta_y(0, delta.y());

    const QSize delta_size_x(delta.x(), 0);
    const QSize delta_size_y(0, delta.y());
    const QSize delta_size(delta.x(), delta.y());

    const QPoint current_element_pos = this->getPosition();
    const QSize current_size = this->getSize();

    QPoint new_position = this->getPosition();
    QSize new_size = this->getSize();

    switch (cursor_state_at_press_)
    {
        case CursorSquareState::LEFT:
            new_position = current_element_pos + delta_x;
            new_size = current_size - delta_size_x;
            break;
        case CursorSquareState::RIGHT:
            new_size = current_size + delta_size_x;
            break;
        case CursorSquareState::TOP:
            new_position = current_element_pos + delta_y;
            new_size = current_size - delta_size_y;
            break;
        case CursorSquareState::BOTTOM:
            new_size = current_size + delta_size_y;
            break;
        case CursorSquareState::INSIDE:
            new_position = current_element_pos + delta;
            new_size = element_size_at_press_;
            break;
        case CursorSquareState::BOTTOM_RIGHT:
            new_size = current_size + delta_size;
            break;
        case CursorSquareState::BOTTOM_LEFT:
            new_position = current_element_pos + delta_x;
            new_size = current_size - delta_size_x + delta_size_y;
            break;
        case CursorSquareState::TOP_RIGHT:
            new_position = current_element_pos + delta_y;
            new_size = current_size + delta_size_x - delta_size_y;
            break;
        case CursorSquareState::TOP_LEFT:
            new_position = current_element_pos + delta;
            new_size = current_size - delta_size_x - delta_size_y;
            break;
        case CursorSquareState::OUTSIDE:
            // Do nothing
            break;
        default:
            std::cout << "Invalid cursor state!" << std::endl;
    }
    if (new_size.width() < 10)
    {
        new_size.setWidth(10);
        new_position.setX(current_element_pos.x());
    }
    if (new_size.height() < 10)
    {
        new_size.setHeight(10);
        new_position.setY(current_element_pos.y());
    }

    const QSize parent_size = this->getParentWidget()->size();
    const float px = parent_size.width();
    const float py = parent_size.height();

    if (new_position.x() < 0)
    {
        if (cursor_state_at_press_ == CursorSquareState::INSIDE)
        {
            new_position.setX(current_element_pos.x());
        }
        else
        {
            new_size.setWidth(current_size.width());
            new_position.setX(current_element_pos.x());
        }
    }
    else if ((new_position.x() + new_size.width()) > px)
    {
        if (cursor_state_at_press_ == CursorSquareState::INSIDE)
        {
            new_position.setX(current_element_pos.x());
        }
        else
        {
            new_size.setWidth(current_size.width());
        }
    }

    if (new_position.y() < 0)
    {
        if (cursor_state_at_press_ == CursorSquareState::INSIDE)
        {
            new_position.setY(current_element_pos.y());
        }
        else
        {
            new_size.setHeight(current_size.height());
            new_position.setY(current_element_pos.y());
        }
    }
    else if ((new_position.y() + new_size.height()) > py)
    {
        if (cursor_state_at_press_ == CursorSquareState::INSIDE)
        {
            new_position.setY(current_element_pos.y());
        }
        else
        {
            new_size.setHeight(current_size.height());
        }
    }

    if ((this->getPosition().x() != new_position.x()) || (this->getPosition().y() != new_position.y()) ||
        (new_size.width() != this->getSize().width()) || (new_size.height() != this->getSize().height()))
    {
        // Plain fraction of the parent's size — no reserved-margin
        // adjustment (see the class-level comment in gui_element.h).
        element_settings_->width = static_cast<float>(new_size.width()) / px;
        element_settings_->height = static_cast<float>(new_size.height()) / py;
        element_settings_->x = static_cast<float>(new_position.x()) / px;
        element_settings_->y = static_cast<float>(new_position.y()) / py;

        setElementPositionAndSize();
        notify_main_window_about_modification_();
    }
}

CursorSquareState GuiElement::getCursorSquareState(const Bound2D bound,
                                                    const Bound2D bound_margin,
                                                    const QPoint mouse_pos)
{
    if (bound.pointIsWithin(mouse_pos))
    {
        if (mouse_pos.x() <= bound_margin.x_min)
        {
            if (mouse_pos.y() <= bound_margin.y_min)
            {
                return CursorSquareState::TOP_LEFT;
            }
            else if (bound_margin.y_max <= mouse_pos.y())
            {
                return CursorSquareState::BOTTOM_LEFT;
            }
            else
            {
                return CursorSquareState::LEFT;
            }
        }
        else if (bound_margin.x_max <= mouse_pos.x())
        {
            if (mouse_pos.y() <= bound_margin.y_min)
            {
                return CursorSquareState::TOP_RIGHT;
            }
            else if (bound_margin.y_max <= mouse_pos.y())
            {
                return CursorSquareState::BOTTOM_RIGHT;
            }
            else
            {
                return CursorSquareState::RIGHT;
            }
        }
        else if (mouse_pos.y() <= bound_margin.y_min)
        {
            return CursorSquareState::TOP;
        }
        else if (bound_margin.y_max <= mouse_pos.y())
        {
            return CursorSquareState::BOTTOM;
        }
        else
        {
            return CursorSquareState::INSIDE;
        }
    }
    else
    {
        return CursorSquareState::OUTSIDE;
    }
}

void GuiElement::setCursorDependingOnMousePos(const QPoint& current_mouse_position)
{
    const auto [bnd, bnd_with_margin] = getBounds();

    const CursorSquareState cms = getCursorSquareState(bnd, bnd_with_margin, current_mouse_position);

    QWidget* const w = this->getParentWidget();

    switch (cms)
    {
        case CursorSquareState::LEFT:
        case CursorSquareState::RIGHT:
            QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
            break;
        case CursorSquareState::TOP:
        case CursorSquareState::BOTTOM:
            QApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
            break;
        case CursorSquareState::BOTTOM_RIGHT:
        case CursorSquareState::TOP_LEFT:
            QApplication::setOverrideCursor(QCursor(Qt::SizeFDiagCursor));
            break;
        case CursorSquareState::BOTTOM_LEFT:
        case CursorSquareState::TOP_RIGHT:
            QApplication::setOverrideCursor(QCursor(Qt::SizeBDiagCursor));
            break;
        case CursorSquareState::INSIDE:
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            break;
        case CursorSquareState::OUTSIDE:
        default:
            QApplication::restoreOverrideCursor();
    }
    (void)w;
}

void GuiElement::setElementPositionAndSize()
{
    const QSize parent_size = this->getParentWidget()->size();

    const float px = parent_size.width();
    const float py = parent_size.height();

    const QSize new_size(static_cast<int>(element_settings_->width * px),
                          static_cast<int>(element_settings_->height * py));

    const QPoint new_pos(static_cast<int>(element_settings_->x * px), static_cast<int>(element_settings_->y * py));

    this->setPosition(new_pos);
    this->setSize(new_size);
}

void GuiElement::updateSizeFromParent(const QSize& /*parent_size*/)
{
    // The wx original recomputes directly from getParent()->GetSize() inside
    // setElementPositionAndSize() rather than trusting a passed-in size, so
    // this does the same rather than using the parameter — kept for
    // interface compatibility with call sites that pass the parent size
    // explicitly (mirroring updateSizeFromParent's call sites in gui_tab.cpp).
    setElementPositionAndSize();
}

void GuiElement::keyPressedCallback(QKeyEvent* evt)
{
    if (!evt->text().isEmpty())
    {
        notify_main_window_key_pressed_(evt->text().toLatin1().at(0));
    }
}

void GuiElement::keyReleasedCallback(QKeyEvent* evt)
{
    if (!evt->text().isEmpty())
    {
        notify_main_window_key_released_(evt->text().toLatin1().at(0));
    }
}

void GuiElement::sendGuiData()
{
    if (element_settings_->handle_string.length() >= 256U)
    {
        throw std::runtime_error("Handle string too long! Maximum length is 255 characters!");
    }

    const std::uint64_t num_bytes_to_send{basicDataSize() + getGuiPayloadSize()};

    FillableUInt8Array output_array{num_bytes_to_send};

    fillWithBasicData(output_array);

    output_array.fillWithStaticType(static_cast<std::uint32_t>(getGuiPayloadSize()));

    fillGuiPayload(output_array);
    lumos::internal::sendThroughTcpInterface(output_array.view(), lumos::internal::kGuiTcpPortNum);
}

void GuiElement::fillWithBasicData(FillableUInt8Array& output_array) const
{
    const std::uint8_t handle_string_length = element_settings_->handle_string.length();

    output_array.fillWithStaticType(static_cast<std::uint8_t>(element_settings_->type));
    output_array.fillWithStaticType(handle_string_length);
    output_array.fillWithDataFromPointer(element_settings_->handle_string.data(),
                                         element_settings_->handle_string.length());
}

std::uint64_t GuiElement::basicDataSize() const
{
    const std::uint8_t handle_string_length = element_settings_->handle_string.length();

    const std::uint64_t basic_data_size = handle_string_length +  // the handle_string itself
                                          sizeof(std::uint8_t) +  // length of handle_string
                                          sizeof(std::uint8_t) +  // type
                                          sizeof(std::uint32_t);  // payload size
    return basic_data_size;
}
