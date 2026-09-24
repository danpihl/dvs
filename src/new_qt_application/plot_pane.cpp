#include "plot_pane.h"

#include <QGuiApplication>
#include <QOpenGLContext>

#include <map>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "axes/axes_side_configuration.h"
#include "lumos/plotting/enumerations.h"
#include "lumos/math.h"
#include "platform_paths.h"

using namespace lumos::internal;

PlotPane::PlotPane(
    QWidget* parent,
    const std::shared_ptr<ElementSettings>& element_settings,
    const RGBTripletf& tab_background_color,
    const GuiCallbacks& callbacks)
    : QOpenGLWidget(parent),
      GuiElement(element_settings, callbacks),
      plot_pane_settings_{std::dynamic_pointer_cast<PlotPaneSettings>(element_settings)},
      tab_background_color_(tab_background_color),
      axes_interactor_(axes_settings_, 1, 1),
      axes_renderer_(nullptr),
      axes_set_(false),
      view_set_(false),
      perspective_projection_(plot_pane_settings_->projection_mode == PlotPaneSettings::ProjectionMode::PERSPECTIVE),
      wait_for_flush_(false),
      plot_data_handler_(nullptr),
      current_mouse_interaction_axis_(MouseInteractionAxis::ALL),
      should_render_point_selection_(false)
{
    axes_interactor_.setViewAnglesSnapAngle(plot_pane_settings_->snap_view_to_axes ? (5.0 * M_PI / 180.0) : 0.0);
    axes_interactor_.setOverriddenMouseInteractionType(MouseInteractionType::UNCHANGED);

    left_mouse_pressed_ = false;

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    LUMOS_LOG_INFO() << "PlotPane created with handle: " << element_settings->handle_string;
}

PlotPane::~PlotPane()
{
    makeCurrent();

    delete axes_renderer_;
    delete plot_data_handler_;

    doneCurrent();

    LUMOS_LOG_INFO() << "PlotPane destroyed";
}

int PlotPane::getWidth()
{
    return this->size().width();
}

int PlotPane::getHeight()
{
    return this->size().height();
}

void PlotPane::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    initShaders();

    axes_renderer_ = new AxesRenderer(shader_collection_, *plot_pane_settings_, tab_background_color_);
    plot_data_handler_ = new PlotDataHandler(shader_collection_);

    axes_set_ = false;
    view_set_ = false;

    LUMOS_LOG_INFO() << "OpenGL initialized in PlotPane";
}

void PlotPane::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    axes_interactor_.updateWindowSize(w, h);
}

void PlotPane::initShaders()
{
    const std::string base_path{getResourcesPathString() + "/shaders/"};

    auto createShader = [&base_path](const std::string& shader_name) -> ShaderBase {
        return ShaderBase{base_path + shader_name + ".vs", base_path + shader_name + ".fs", ShaderSource::FILE};
    };

    shader_collection_.plot_box_shader = createShader("plot_box_shader");
    shader_collection_.pane_background_shader = createShader("pane_background");
    shader_collection_.text_shader = TextShader{base_path + "text.vs", base_path + "text.fs", ShaderSource::FILE};
    shader_collection_.basic_plot_shader = createShader("basic_plot_shader");
    shader_collection_.plot_2d_shader =
        Plot2DShader{base_path + "plot_2d_shader.vs", base_path + "plot_2d_shader.fs", ShaderSource::FILE};
    shader_collection_.plot_3d_shader =
        Plot3DShader{base_path + "plot_3d_shader.vs", base_path + "plot_3d_shader.fs", ShaderSource::FILE};
    shader_collection_.img_plot_shader = ImShowShader{base_path + "img.vs", base_path + "img.fs", ShaderSource::FILE};
    shader_collection_.scatter_shader =
        ScatterShader{base_path + "scatter_shader.vs", base_path + "scatter_shader.fs", ShaderSource::FILE};
    shader_collection_.draw_mesh_shader =
        DrawMeshShader{base_path + "draw_mesh_shader.vs", base_path + "draw_mesh_shader.fs", ShaderSource::FILE};
    shader_collection_.legend_shader = createShader("legend_shader");
    shader_collection_.screen_space_shader = createShader("screen_space_shader");
}

void PlotPane::paintGL()
{
    const RGBTripletf& bg = plot_pane_settings_->background_color;
    glClearColor(bg.red, bg.green, bg.blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    processActionQueue();

    const AxesSideConfiguration axes_side_configuration{axes_interactor_.getViewAngles(), perspective_projection_};

    axes_renderer_->updateStates(axes_interactor_.getAxesLimits(),
                                 axes_interactor_.getViewAngles(),
                                 axes_interactor_.getQueryPoint(),
                                 axes_interactor_.generateGridVectors(),
                                 axes_side_configuration,
                                 perspective_projection_,
                                 static_cast<float>(width()),
                                 static_cast<float>(height()),
                                 Vec2f(0.0f, 0.0f),
                                 Vec2f(0.0f, 0.0f),
                                 axes_interactor_.getMouseInteractionType(),
                                 left_mouse_pressed_,
                                 axes_interactor_.shouldDrawZoomRect(),
                                 axes_interactor_.getShowLegend(),
                                 1.0f,
                                 plot_data_handler_->getLegendStrings(),
                                 "plot_pane",
                                 current_mouse_interaction_axis_);

    axes_renderer_->render();
    glEnable(GL_DEPTH_TEST);
    axes_renderer_->plotBegin();

    plot_data_handler_->render();

    axes_renderer_->plotEnd();

    glDisable(GL_DEPTH_TEST);
}

void PlotPane::processActionQueue()
{
    while (!queued_data_.empty())
    {
        const Function fcn = queued_data_.front()->getFunction();

        if (fcn == Function::AXES_2D || fcn == Function::AXES_3D)
        {
            axes_set_ = true;
            auto [received_data, plot_object_attributes, user_supplied_properties] =
                queued_data_.front()->moveAllDataButConvertedData();

            const CommunicationHeader& hdr = received_data.getCommunicationHeader();
            const std::pair<Vec3d, Vec3d> axes_bnd =
                hdr.get(CommunicationHeaderObjectType::AXIS_MIN_MAX_VEC).as<std::pair<Vec3d, Vec3d>>();

            if (fcn == Function::AXES_2D)
            {
                axes_interactor_.setAxesLimits(Vec2d(axes_bnd.first.x, axes_bnd.first.y),
                                               Vec2d(axes_bnd.second.x, axes_bnd.second.y));
            }
            else
            {
                axes_interactor_.setAxesLimits(Vec3d(axes_bnd.first.x, axes_bnd.first.y, axes_bnd.first.z),
                                               Vec3d(axes_bnd.second.x, axes_bnd.second.y, axes_bnd.second.z));
            }
        }
        else if (fcn == Function::VIEW)
        {
            view_set_ = true;
            auto [received_data, plot_object_attributes, user_supplied_properties] =
                queued_data_.front()->moveAllDataButConvertedData();

            const CommunicationHeader& hdr = received_data.getCommunicationHeader();
            const float azimuth = hdr.get(CommunicationHeaderObjectType::AZIMUTH).as<float>();
            const float elevation = hdr.get(CommunicationHeaderObjectType::ELEVATION).as<float>();

            axes_interactor_.setViewAngles(azimuth * M_PI / 180.0f, elevation * M_PI / 180.0f);
        }
        else if (fcn == Function::CLEAR)
        {
            // Matches wx exactly (main_application/plot_pane.cpp:286-289,
            // via addSettingsData): clear immediately, synchronously, right
            // here — not via a "pending" flag applied on some later frame.
            // A deferred flag lets any plot data queued alongside/after the
            // same CLEAR message get added first and then wiped on the next
            // repaint, whenever that happens to occur (a `pending_clear_`
            // flag was tried here and had exactly that bug — see
            // CURRENT_STATE.md).
            clearPane();
        }
        else if (isPlotDataFunction(fcn))
        {
            auto [received_data, plot_object_attributes, user_supplied_properties, converted_data] =
                queued_data_.front()->moveAllData();

            const CommunicationHeader& hdr = received_data.getCommunicationHeader();
            plot_data_handler_->addData(hdr, plot_object_attributes, user_supplied_properties, received_data,
                                        converted_data);
        }

        queued_data_.pop();
    }

    if (axes_set_ && view_set_)
    {
        axes_interactor_.updateWindowSize(width(), height());
    }
}

void PlotPane::clearPane()
{
    plot_data_handler_->clear();
    axes_set_ = false;
    view_set_ = false;
}

// ---------------------------------------------------------------------------
// Mouse handling. Left button and right button are routed through
// GuiElement's edit-mode dispatch (mouseLeftPressed/mouseMovedOverItem/
// mouseLeftReleased, mouseRightPressed/mouseRightReleased) exactly as the wx
// original binds wxEVT_LEFT_DOWN/MOTION/LEFT_UP and wxEVT_RIGHT_DOWN/UP
// directly to ApplicationGuiElement's handlers, so a Ctrl(=Meta on macOS)
// click-drag always means "resize/move this element" first, falling through
// to camera interaction (*GuiElementSpecific) only when it isn't. Middle
// button has no edit-mode meaning in the wx original either, so it's wired
// straight to mouseMiddlePressed/Released, matching wx's own direct Bind.
// ---------------------------------------------------------------------------

void PlotPane::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        mouseLeftPressed(event);
    }
    else if (event->button() == Qt::MiddleButton)
    {
        mouseMiddlePressed(event);
    }
    else if (event->button() == Qt::RightButton)
    {
        mouseRightPressed(event);
    }
}

void PlotPane::mouseMoveEvent(QMouseEvent* event)
{
    mouseMovedOverItem(event);
}

void PlotPane::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        mouseLeftReleased(event);
    }
    else if (event->button() == Qt::MiddleButton)
    {
        mouseMiddleReleased(event);
    }
    else if (event->button() == Qt::RightButton)
    {
        mouseRightReleased(event);
    }
}

void PlotPane::wheelEvent(QWheelEvent* /*event*/)
{
    // Mouse-wheel zoom has no equivalent in the wx original (zoom is
    // shift+right-drag there) — matches upstream, not implemented.
    QOpenGLWidget::update();
}

void PlotPane::keyPressEvent(QKeyEvent* event)
{
    held_keys_.insert(event->key());
    keyPressedCallback(event);
    keyPressed(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
}

void PlotPane::keyReleaseEvent(QKeyEvent* event)
{
    held_keys_.erase(event->key());
    keyReleasedCallback(event);
    keyReleased(event->key() < 256 ? static_cast<char>(event->key()) : '\0');
}

void PlotPane::enterEvent(QEnterEvent* event)
{
    mouseEnteredElement(event);
}

void PlotPane::leaveEvent(QEvent* event)
{
    mouseLeftElement(event);
}

void PlotPane::mouseMiddlePressed(QMouseEvent* /*event*/)
{
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
    {
        shift_pressed_at_mouse_press_ = true;
        axes_interactor_.setOverriddenMouseInteractionType(MouseInteractionType::PAN);
    }
}

void PlotPane::mouseMiddleReleased(QMouseEvent* /*event*/)
{
    if (shift_pressed_at_mouse_press_)
    {
        shift_pressed_at_mouse_press_ = false;
        axes_interactor_.setOverriddenMouseInteractionType(MouseInteractionType::UNCHANGED);
    }
    QOpenGLWidget::update();
}

void PlotPane::mouseLeftPressedGuiElementSpecific(QMouseEvent* event)
{
    const QPoint mouse_pos_at_press_local = event->pos();

    left_mouse_pressed_ = true;

    const Vec2f mouse_pos_at_press_normalized{
        Vec2f(mouse_pos_at_press_local.x(), mouse_pos_at_press_local.y())
            .elementWiseDivide(Vec2f(element_size_at_press_.width(), element_size_at_press_.height()))};

    axes_interactor_.registerMousePressed(mouse_pos_at_press_normalized);

    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
    {
        shift_pressed_at_mouse_press_ = true;
        axes_interactor_.setOverriddenMouseInteractionType(MouseInteractionType::ROTATE);
    }

    QOpenGLWidget::update();
}

void PlotPane::mouseLeftReleasedGuiElementSpecific(QMouseEvent* event)
{
    const QPoint mouse_local_pos = event->pos();
    const QSize size_now = this->size();

    axes_interactor_.registerMouseReleased(
        Vec2f(mouse_local_pos.x(), mouse_local_pos.y()).elementWiseDivide(Vec2f(size_now.width(), size_now.height())));

    left_mouse_pressed_ = false;

    if (shift_pressed_at_mouse_press_)
    {
        shift_pressed_at_mouse_press_ = false;
        axes_interactor_.setOverriddenMouseInteractionType(MouseInteractionType::UNCHANGED);
    }

    QOpenGLWidget::update();
}

void PlotPane::mouseRightPressedGuiElementSpecific(QMouseEvent* /*event*/)
{
    if (QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
    {
        axes_interactor_.setOverriddenMouseInteractionType(MouseInteractionType::ZOOM);
    }
}

void PlotPane::mouseRightLeftGuiElementSpecific(QMouseEvent* event)
{
    if (event->buttons() & Qt::RightButton)
    {
        if (shift_pressed_at_mouse_press_)
        {
            shift_pressed_at_mouse_press_ = false;
            axes_interactor_.setOverriddenMouseInteractionType(MouseInteractionType::UNCHANGED);
        }
        QOpenGLWidget::update();
    }
}

void PlotPane::mouseMovedGuiElementSpecific(QMouseEvent* event)
{
    const QPoint current_mouse_position_local = event->pos();

    if (keyHeld(Qt::Key_L) && (event->buttons() & Qt::LeftButton))
    {
        const Vec2d diff(current_mouse_pos_.x() - previous_mouse_pos_.x(),
                          current_mouse_pos_.y() - previous_mouse_pos_.y());
        Vec3d scale_factor = axes_renderer_->getAxesBoxScaleFactor();
        scale_factor = scale_factor * (diff.x < 0 ? 0.99 : 1.01);
        axes_renderer_->setAxesBoxScaleFactor(scale_factor);
    }
    else if (event->buttons() & (Qt::LeftButton | Qt::MiddleButton | Qt::RightButton))
    {
        if ((event->buttons() & Qt::LeftButton) && control_pressed_at_mouse_press_)
        {
            adjustPaneSizeOnMouseMoved();
        }
        else if (shift_pressed_at_mouse_press_ || (event->buttons() & Qt::LeftButton))
        {
            axes_interactor_.registerMouseDragInput(current_mouse_interaction_axis_,
                                                    current_mouse_position_local.x(),
                                                    current_mouse_position_local.y(),
                                                    current_mouse_pos_.x() - previous_mouse_pos_.x(),
                                                    current_mouse_pos_.y() - previous_mouse_pos_.y());
        }

        QOpenGLWidget::update();
    }
}

void PlotPane::setMouseInteractionType(const MouseInteractionType mit)
{
    axes_interactor_.setMouseInteractionType(mit);
}

void PlotPane::keyPressedElementSpecific(const char /*key*/)
{
    if (keyHeld(Qt::Key_R))
    {
        axes_interactor_.setMouseInteractionType(MouseInteractionType::ROTATE);
    }
    else if (keyHeld(Qt::Key_Z))
    {
        axes_interactor_.setMouseInteractionType(MouseInteractionType::ZOOM);
    }
    else if (keyHeld(Qt::Key_P))
    {
        axes_interactor_.setMouseInteractionType(MouseInteractionType::PAN);
    }

    QOpenGLWidget::update();
}

void PlotPane::keyReleasedElementSpecific(const char /*key*/)
{
    current_mouse_interaction_axis_ = MouseInteractionAxis::ALL;

    if (keyHeld(Qt::Key_1) && keyHeld(Qt::Key_2))
    {
        current_mouse_interaction_axis_ = MouseInteractionAxis::XY;
    }
    else if (keyHeld(Qt::Key_2) && keyHeld(Qt::Key_3))
    {
        current_mouse_interaction_axis_ = MouseInteractionAxis::YZ;
    }
    else if (keyHeld(Qt::Key_1) && keyHeld(Qt::Key_3))
    {
        current_mouse_interaction_axis_ = MouseInteractionAxis::XZ;
    }
    else if (keyHeld(Qt::Key_1))
    {
        current_mouse_interaction_axis_ = MouseInteractionAxis::X;
    }
    else if (keyHeld(Qt::Key_2))
    {
        current_mouse_interaction_axis_ = MouseInteractionAxis::Y;
    }
    else if (keyHeld(Qt::Key_3))
    {
        current_mouse_interaction_axis_ = MouseInteractionAxis::Z;
    }
}

void PlotPane::pushQueue(std::queue<std::unique_ptr<InputData>>& new_queue)
{
    while (!new_queue.empty())
    {
        queued_data_.push(std::move(new_queue.front()));
        new_queue.pop();
    }
    repaintIfNewData();
}

void PlotPane::addPlotData(ReceivedData& received_data,
                           const PlotObjectAttributes& plot_object_attributes,
                           const UserSuppliedProperties& user_supplied_properties,
                           const std::shared_ptr<const ConvertedDataBase>& converted_data)
{
    const CommunicationHeader& hdr = received_data.getCommunicationHeader();
    plot_data_handler_->addData(hdr, plot_object_attributes, user_supplied_properties, received_data, converted_data);
    QOpenGLWidget::update();
}

void PlotPane::addSettingsData(const ReceivedData& received_data,
                               const PlotObjectAttributes& /*plot_object_attributes*/,
                               const UserSuppliedProperties& /*user_supplied_properties*/)
{
    const CommunicationHeader& hdr = received_data.getCommunicationHeader();
    const Function fcn = hdr.getFunction();

    if (fcn == Function::AXES_2D)
    {
        axes_set_ = true;
        const std::pair<Vec3d, Vec3d> axes_bnd =
            hdr.get(CommunicationHeaderObjectType::AXIS_MIN_MAX_VEC).as<std::pair<Vec3d, Vec3d>>();
        axes_interactor_.setAxesLimits(Vec2d(axes_bnd.first.x, axes_bnd.first.y),
                                       Vec2d(axes_bnd.second.x, axes_bnd.second.y));
    }
}

void PlotPane::repaintIfNewData()
{
    if (!queued_data_.empty())
    {
        QOpenGLWidget::update();
    }
}

void PlotPane::refresh()
{
    QOpenGLWidget::update();
}

void PlotPane::waitForFlush()
{
    wait_for_flush_ = true;
}

void PlotPane::toggleProjectionMode()
{
    perspective_projection_ = !perspective_projection_;
    plot_pane_settings_->projection_mode =
        perspective_projection_ ? PlotPaneSettings::ProjectionMode::PERSPECTIVE : PlotPaneSettings::ProjectionMode::ORTHOGRAPHIC;
    QOpenGLWidget::update();
}

void PlotPane::setHandleString(const std::string& new_name)
{
    plot_pane_settings_->handle_string = new_name;
    QOpenGLWidget::update();
}

void PlotPane::updateElementSettings(const std::map<std::string, std::string>& new_settings)
{
    plot_pane_settings_->handle_string = new_settings.at("handle_string");
    if (new_settings.count("title") > 0U)
    {
        plot_pane_settings_->title = new_settings.at("title");
    }
}
