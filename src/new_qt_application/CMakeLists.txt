cmake_minimum_required(VERSION 3.14.3 FATAL_ERROR)

project(new-qt-app CXX)

# Phase 1 scaffold: framework-independent modules copied fresh from
# main_application (see AUDIT_OF_PRIOR_ATTEMPT.md for why this is a fresh
# copy rather than a continuation of src/qt_application). Built as a static
# library so it can be validated in isolation before any Qt-specific GUI
# code (MainWindow/GuiWindow/WindowTab/PlotPane/GuiElements — Phase 2+) is
# written on top of it. Phase 2 adds the Qt-specific sources to the same
# library as they're written.

set(CMAKE_AUTOMOC ON)

include_directories(${REPO_DIR}/src/new_qt_application)
include_directories(${REPO_DIR}/third_party/LumosAlgo/src)

if(PLATFORM_APPLE)
    include_directories(${REPO_DIR}/src/externals/harfbuzz/build/include/harfbuzz)
endif()

set(CORE_SOURCE_FILES
    axes/axes_interactor.cpp
    axes/axes_renderer.cpp
    axes/plot_box_walls.cpp
    axes/plot_box_silhouette.cpp
    axes/plot_pane_background.cpp
    axes/zoom_rect.cpp
    axes/grid_numbers.cpp
    axes/plot_box_grid.cpp
    axes/text_rendering.cpp
    axes/text_rendering_impl/font_atlas.cpp
    axes/text_rendering_impl/label_text_store.cpp
    axes/text_rendering_impl/buffers/VertexBufferLayout.cpp
    axes/text_rendering_impl/buffers/VertexBuffer.cpp
    axes/text_rendering_impl/buffers/VertexArray.cpp
    axes/text_rendering_impl/buffers/IndexBuffer.cpp
    axes/text_rendering_impl/buffers/gBuffers.cpp
    axes/legend_renderer.cpp
    axes/point_selection_box.cpp
    axes/axes_side_configuration.cpp
    axes/structures/axes_limits.cpp
    axes/structures/axes_settings.cpp
    axes/structures/view_angles.cpp

    project_state/project_settings.cpp
    project_state/element_settings.cpp
    project_state/plot_pane_settings.cpp
    project_state/other_gui_settings.cpp
    project_state/configuration_agent.cpp
    project_state/scrolling_text_settings.cpp

    shader.cpp

    misc/number_formatting.cpp

    plot_objects/plot_object_base/plot_object_base.cpp
    plot_objects/draw_mesh/draw_mesh.cpp
    plot_objects/im_show/im_show.cpp
    plot_objects/plot_collection2/plot_collection2.cpp
    plot_objects/plot_collection3/plot_collection3.cpp
    plot_objects/plot2d/plot2d.cpp
    plot_objects/plot3d/plot3d.cpp
    plot_objects/fast_plot2d/fast_plot2d.cpp
    plot_objects/fast_plot3d/fast_plot3d.cpp
    plot_objects/line_collection2/line_collection2.cpp
    plot_objects/line_collection3/line_collection3.cpp
    plot_objects/stem/stem.cpp
    plot_objects/scatter/scatter.cpp
    plot_objects/scatter3/scatter3.cpp
    plot_objects/stairs/stairs.cpp
    plot_objects/surf/surf.cpp
    plot_objects/scrolling_plot2d/scrolling_plot2d.cpp
    plot_objects/screen_space_primitive/screen_space_primitive.cpp
    plot_objects/stream_object_base/stream_object_base.cpp
    plot_objects/stream_objects/plot2d/plot2d.cpp
    plot_objects/stream_objects/scatter/scatter.cpp
    plot_objects/stream_objects/stairs/stairs.cpp

    globals.cpp

    communication/received_data.cpp
    communication/data_receiver.cpp
    input_data.cpp
    point_selection.cpp

    serial_interface/serial_port.cpp
    serial_interface/serial_interface.cpp
    serial_interface/raw_data_frame.cpp

    plot_data_handler.cpp
    color_picker.cpp
    platform_paths.cpp
    user_supplied_properties.cpp
    opengl_debug.cpp

    # Phase 2: Qt-specific GUI layer, written fresh (see AUDIT_OF_PRIOR_ATTEMPT.md
    # for why src/qt_application's versions of these are not reused directly).
    gui_element.cpp
    plot_pane.cpp
    editing_silhouette.cpp
    gui_elements.cpp
    window_tab.cpp
    gui_window.cpp
    main_window.cpp
    message_router.cpp
    window_manager.cpp
    project_controller.cpp
    settings_dialog.cpp
    preferences_dialog.cpp
)

add_library(new_qt_duoplot_core STATIC ${CORE_SOURCE_FILES})
target_link_libraries(new_qt_duoplot_core Qt6::Core Qt6::Widgets Qt6::OpenGLWidgets)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${REPO_DIR}/src/build/)

add_executable(new_qt_duoplot main.cpp)
target_link_libraries(new_qt_duoplot new_qt_duoplot_core Qt6::Core Qt6::Widgets Qt6::OpenGLWidgets
                       ${REPO_DIR}/src/externals/harfbuzz/build/lib/libharfbuzz.a
                       ${REPO_DIR}/src/externals/freetype/build/libfreetype.a)
