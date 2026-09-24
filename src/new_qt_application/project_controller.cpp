#include "project_controller.h"

#include <QFileDialog>
#include <QMessageBox>

#include <optional>

#include "filesystem.h"
#include "lumos/logging.h"
#include "window_manager.h"

ProjectController::ProjectController(QWidget* dialog_parent, ConfigurationAgent* configuration_agent)
    : dialog_parent_{dialog_parent},
      configuration_agent_{configuration_agent},
      window_manager_{nullptr},
      save_manager_{nullptr},
      window_initialization_in_progress_{true}
{
    // Faithful port of the wx constructor's SaveManager setup
    // (main_application/main_window.cpp:142-150).
    const std::optional<std::string> last_opened_file = configuration_agent_->getLastOpenedFile();
    if (last_opened_file.has_value() && lumos::filesystem::exists(*last_opened_file))
    {
        save_manager_ = new SaveManager(*last_opened_file);
    }
    else
    {
        save_manager_ = new SaveManager();
    }
}

ProjectController::~ProjectController()
{
    delete save_manager_;
}

void ProjectController::setWindowManager(WindowManager* window_manager)
{
    window_manager_ = window_manager;
}

SaveManager* ProjectController::getSaveManager() const
{
    return save_manager_;
}

void ProjectController::beginInitialization()
{
    window_initialization_in_progress_ = true;
}

void ProjectController::endInitialization()
{
    window_initialization_in_progress_ = false;
}

void ProjectController::fileModified()
{
    if (window_initialization_in_progress_)
    {
        return;
    }
    save_manager_->setIsModified();
    window_manager_->setIsFileSavedForAllWindows(false);
}

void ProjectController::newProject()
{
    beginInitialization();

    if (!save_manager_->isSaved())
    {
        const auto reply = QMessageBox::question(dialog_parent_, "Please confirm",
                                                  "Current content has not been saved! Proceed?",
                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
        {
            endInitialization();
            return;
        }
    }

    save_manager_->reset();
    window_manager_->resetForNewProject();

    endInitialization();
}

void ProjectController::saveProject()
{
    if (!save_manager_->savePathIsSet())
    {
        saveProjectAs();
        return;
    }

    if (save_manager_->isSaved())
    {
        return;
    }

    const ProjectSettings ps = window_manager_->getCurrentProjectSettings();

    configuration_agent_->setLastOpenedFile(save_manager_->getCurrentFilePath());

    save_manager_->save(ps);
    window_manager_->setIsFileSavedForAllWindows(true);
}

void ProjectController::saveProjectAs(const std::string& file_path)
{
    configuration_agent_->setLastOpenedFile(file_path);

    if (file_path == save_manager_->getCurrentFilePath())
    {
        saveProject();
        return;
    }

    const ProjectSettings ps = window_manager_->getCurrentProjectSettings();

    save_manager_->saveToNewFile(file_path, ps);

    window_manager_->setProjectNameForAllWindows(save_manager_->getCurrentFileName());
    window_manager_->setIsFileSavedForAllWindows(true);
}

void ProjectController::saveProjectAs()
{
    const QString path =
        QFileDialog::getSaveFileName(dialog_parent_, "Choose file to save to", "", "duoplot files (*.duoplot)");
    if (path.isEmpty())
    {
        return;
    }
    saveProjectAs(path.toStdString());
}

bool ProjectController::openExistingFile(const std::string& file_path)
{
    beginInitialization();

    if (save_manager_->getCurrentFilePath() == file_path)
    {
        endInitialization();
        return false;
    }

    // Parse before touching any existing window/element state. wx's own
    // MainWindow::openExistingFile() calls removeAllWindows() unconditionally,
    // before attempting the load, so a file that fails to parse still tears
    // down the current project (ProjectSettings's file-loading constructor
    // swallows the parse exception and just leaves an empty project). That
    // also orphans any plot data already queued for the current project's
    // elements at the moment the client requested the file switch — see
    // ARCHITECTURE_IMPROVEMENTS.md #6. Deliberate improvement over the wx
    // original: check the file loads before committing to replacing the
    // current project, so a bad path leaves everything untouched.
    const ProjectSettings prospective_settings{file_path};
    if (!prospective_settings.isValid())
    {
        LUMOS_LOG_WARNING() << "Failed to open project file '" << file_path
                             << "' — current project left unchanged.";
        endInitialization();
        return false;
    }

    window_manager_->removeAllWindows();

    configuration_agent_->setLastOpenedFile(file_path);
    save_manager_->openExistingFile(file_path);

    window_manager_->setupWindows(save_manager_->getCurrentProjectSettings());

    endInitialization();
    return true;
}

void ProjectController::openExistingFile()
{
    if (!save_manager_->isSaved())
    {
        const auto reply = QMessageBox::question(dialog_parent_, "Please confirm",
                                                  "Current content has not been saved! Proceed?",
                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
        {
            return;
        }
    }

    const QString path =
        QFileDialog::getOpenFileName(dialog_parent_, "Choose file to open", "", "duoplot files (*.duoplot)");
    if (path.isEmpty())
    {
        return;
    }
    openExistingFile(path.toStdString());
}
