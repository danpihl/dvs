#include <QApplication>
#include <QSurfaceFormat>

#include "lumos/logging.h"
#include "main_window.h"

int main(int argc, char* argv[])
{
    // Request OpenGL 3.3 Core Profile globally before creating QApplication —
    // required on macOS (Qt otherwise defaults to a legacy 2.1 context that
    // doesn't support the GLSL #version 330 shaders under axes/opengl_low_level).
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setApplicationName("Duoplot");
    app.setOrganizationName("Duoplot");

    LUMOS_LOG_INFO() << "Starting new_qt_duoplot";

    std::vector<std::string> cmd_args;
    for (int k = 0; k < argc; k++)
    {
        cmd_args.emplace_back(argv[k]);
    }

    MainWindow main_window(cmd_args);

    return app.exec();
}
