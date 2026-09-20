#ifndef MAIN_APPLICATION_PROJECT_STATE_SAVE_MANAGER_H_
#define MAIN_APPLICATION_PROJECT_STATE_SAVE_MANAGER_H_

#include <iomanip>
#include <nlohmann/json.hpp>
#include <string>

#include "project_state/project_settings.h"

class SaveManager
{
private:
    std::string file_path_;
    ProjectSettings project_settings_;

    bool is_saved_;
    bool save_path_is_set_;

public:
    SaveManager()
    {
        this->reset();
    }

    SaveManager(const std::string& file_path)
        : file_path_{file_path}, project_settings_{file_path}, is_saved_{true}, save_path_is_set_{true}
    {
        file_path_ = file_path;
        project_settings_ = ProjectSettings(file_path);
        is_saved_ = true;
        save_path_is_set_ = true;
    }

    void setIsModified()
    {
        is_saved_ = false;
    }

    bool isSaved() const
    {
        return is_saved_;
    }

    bool savePathIsSet() const
    {
        return save_path_is_set_;
    }

    void reset()
    {
        save_path_is_set_ = false;
        file_path_ = "Untitled";
        is_saved_ = true;
        project_settings_ = ProjectSettings();
    }

    std::string getCurrentFileName() const
    {
        if (!save_path_is_set_)
        {
            return "Untitled";
        }
        else
        {
            size_t last = 0;
            size_t next = 0;
            while ((next = file_path_.find("/", last)) != std::string::npos)
            {
                last = next + 1;
            }

            return file_path_.substr(last);
        }
    }

    void save(const ProjectSettings& changed_project_settings)
    {
        if (changed_project_settings != project_settings_)
        {
            const nlohmann::json j_to_save = changed_project_settings.toJson();

            std::ofstream output_file(file_path_);
            output_file << std::setw(4) << j_to_save << std::endl;

            project_settings_ = changed_project_settings;

            is_saved_ = true;
        }
    }

    std::string getCurrentFilePath() const
    {
        return file_path_;
    }

    void openExistingFile(const std::string& file_path)
    {
        file_path_ = file_path;
        project_settings_ = ProjectSettings(file_path);
        save_path_is_set_ = true;
        is_saved_ = true;
    }

    void saveToNewFile(const std::string& file_path, const ProjectSettings& changed_project_settings)
    {
        file_path_ = file_path;
        project_settings_ = changed_project_settings;
        save_path_is_set_ = true;

        const nlohmann::json j_to_save = changed_project_settings.toJson();

        std::ofstream output_file(file_path_);
        output_file << std::setw(4) << j_to_save << std::endl;

        project_settings_ = changed_project_settings;

        is_saved_ = true;
    }

    ProjectSettings getCurrentProjectSettings() const
    {
        return project_settings_;
    }
};

#endif  // MAIN_APPLICATION_PROJECT_STATE_SAVE_MANAGER_H_
