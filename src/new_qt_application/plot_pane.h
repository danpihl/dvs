#ifndef NEW_QT_APPLICATION_PLOT_PANE_H_
#define NEW_QT_APPLICATION_PLOT_PANE_H_

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

#include <atomic>
#include <queue>
#include <set>

#include "axes/axes_interactor.h"
#include "axes/axes_renderer.h"
#include "axes/structures/axes_settings.h"
#include "communication/received_data.h"
#include "gui_element.h"
#include "input_data.h"
#include "plot_data_handler.h"
#include "point_selection.h"
#include "project_state/project_settings.h"
#include "shader.h"

// Faithful port of main_application/plot_pane.{h,cpp}'s PlotPane.
//
// Deferred, not dropped: the wx original also subscribes each pane to
// text_stream_objects "topic" streams (initSubscribedStreams/pushStreamData,
// rendered inline in PlotPane::render) — a separate feature tied to
// MainWindow's serial/stream-of-strings plumbing. Not ported yet; see
// CURRENT_STATE.md. Everything else (rendering pipeline, mouse camera
// interaction, edit-mode resize via GuiElement, keyboard axis constraints)
// is a faithful port.
class PlotPane : public QOpenGLWidget, public GuiElement, protected QOpenGLFunctions
{
    Q_OBJECT

private:
    std::shared_ptr<PlotPaneSettings> plot_pane_settings_;
    RGBTripletf tab_background_color_;

    AxesSettings axes_settings_;
    AxesInteractor axes_interactor_;
    AxesRenderer* axes_renderer_;
    bool axes_set_;
    bool view_set_;
    bool perspective_projection_;
    bool wait_for_flush_;

    PlotDataHandler* plot_data_handler_;
    PointSelection point_selection_;
    ShaderCollection shader_collection_;

    MouseInteractionAxis current_mouse_interaction_axis_;
    bool should_render_point_selection_;

    std::queue<std::unique_ptr<InputData>> queued_data_;

    // wx queries physical key state at any time via wxGetKeyState(), which
    // has no direct Qt equivalent; this widget's own keyPressEvent/
    // keyReleaseEvent track the keys used by the wx original's key-combo
    // behavior (axis-constrained rotation, 'l'+drag box scale) so
    // keyPressedElementSpecific/mouseMovedGuiElementSpecific can query them
    // the same way. Only meaningful while this widget has focus.
    std::set<int> held_keys_;
    bool keyHeld(int qt_key) const
    {
        return held_keys_.count(qt_key) > 0;
    }

    void initShaders();
    void processActionQueue();
    void clearPane();

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

    void mouseMiddlePressed(QMouseEvent* event);
    void mouseMiddleReleased(QMouseEvent* event);

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

    std::uint64_t getGuiPayloadSize() const override
    {
        return 0U;
    }

    void fillGuiPayload(FillableUInt8Array& /*output_array*/) const override {}

public:
    PlotPane(QWidget* parent,
             const std::shared_ptr<ElementSettings>& element_settings,
             const RGBTripletf& tab_background_color,
             const std::function<void(const char key)>& notify_main_window_key_pressed,
             const std::function<void(const char key)>& notify_main_window_key_released,
             const std::function<void(const QPoint pos, const std::string& elem_name)>&
                 notify_parent_window_right_mouse_pressed,
             const std::function<void()>& notify_main_window_about_modification,
             const std::function<void(const QPoint& pos, const QSize& size, const bool is_editing)>&
                 notify_tab_about_editing,
             const std::function<void(const Color_t, const std::string&)>& push_text_to_cmdl_output_window);
    ~PlotPane() override;

    // QWidget::show()/hide() and GuiElement::show()/hide() are both plain
    // public members with the same name, ambiguous for any class — like
    // this one — that inherits both. Disambiguate once here rather than at
    // every call site.
    void show() override
    {
        QOpenGLWidget::show();
    }

    void hide() override
    {
        QOpenGLWidget::hide();
    }

    int getWidth();
    int getHeight();

    void setHandleString(const std::string& new_name) override;
    void pushQueue(std::queue<std::unique_ptr<InputData>>& new_queue);

    void addSettingsData(const ReceivedData& received_data,
                         const PlotObjectAttributes& plot_object_attributes,
                         const UserSuppliedProperties& user_supplied_properties);
    void addPlotData(ReceivedData& received_data,
                     const PlotObjectAttributes& plot_object_attributes,
                     const UserSuppliedProperties& user_supplied_properties,
                     const std::shared_ptr<const ConvertedDataBase>& converted_data);

    void refresh();
    void setMouseInteractionType(const MouseInteractionType mit);
    void keyPressedElementSpecific(const char key) override;
    void keyReleasedElementSpecific(const char key) override;
    void waitForFlush();
    void toggleProjectionMode();

    // Renamed from an earlier `update()` override: naming it the same as
    // QOpenGLWidget::update() silently hid the real repaint trigger for
    // every unqualified `update()` call inside this class, including the
    // mouse/keyboard interaction handlers that need an unconditional
    // repaint (not gated on new data) — see CURRENT_STATE.md for the bug
    // this caused. Only pushQueue calls this; everything else calls the
    // inherited QOpenGLWidget::update() directly.
    void repaintIfNewData();

    void mouseRightPressedGuiElementSpecific(QMouseEvent* event) override;
    void mouseRightLeftGuiElementSpecific(QMouseEvent* event) override;

    void updateElementSettings(const std::map<std::string, std::string>& new_settings) override;

    void mouseMovedGuiElementSpecific(QMouseEvent* event) override;
    void mouseLeftPressedGuiElementSpecific(QMouseEvent* event) override;
    void mouseLeftReleasedGuiElementSpecific(QMouseEvent* event) override;
};

#endif  // NEW_QT_APPLICATION_PLOT_PANE_H_
