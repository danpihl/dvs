#include "project_state/configuration_agent.h"

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iomanip>

#include "platform_paths.h"

ConfigurationAgent::~ConfigurationAgent() {}

ConfigurationAgent::ConfigurationAgent() : is_valid_{true}
{
    const lumos::filesystem::path duoplot_dir_path{getConfigDir()};

    if (!lumos::filesystem::exists(duoplot_dir_path))
    {
        LUMOS_LOG_INFO() << "duoplot dir does not exist, creating...";
        try
        {
            lumos::filesystem::create_directory(duoplot_dir_path);
        }
        catch (const std::exception& e)
        {
            LUMOS_LOG_ERROR() << "Failed to create duoplot directory at \"" << duoplot_dir_path << "\". Exception: " << e.what();
            is_valid_ = false;
            return;
        }
    }
    configuration_file_path_ = lumos::filesystem::path(duoplot_dir_path / "configuration.json");

    if (!lumos::filesystem::exists(configuration_file_path_))
    {
        LUMOS_LOG_INFO() << "The file \"" << configuration_file_path_ << "\" file does not exist, creating...";
        try
        {
            createEmptyConfigurationFile();
        }
        catch (const std::exception& e)
        {
            LUMOS_LOG_ERROR() << "Failed to create configuration file at \"" << configuration_file_path_
                            << "\". Exception: " << e.what();
            is_valid_ = false;
            return;
        }
    }
    else
    {
        bool reading_successful{false};
        try
        {
            readConfigurationFile();
            reading_successful = true;
        }
        catch (const std::exception& e)
        {
            LUMOS_LOG_WARNING() << "Error reading existing configuration file \"" << configuration_file_path_
                              << "\". Exception: " << e.what() << ". Creating a new one.";
        }

        if (!reading_successful)
        {
            try
            {
                createEmptyConfigurationFile();
            }
            catch (const std::exception& e)
            {
                LUMOS_LOG_ERROR() << "Failed to create configuration.json at \"" << configuration_file_path_
                                << "\". Exception: " << e.what() << ". Exiting.";
                is_valid_ = false;
                return;
            }
        }
    }
}

bool ConfigurationAgent::isValid() const
{
    return is_valid_;
}

bool ConfigurationAgent::hasKey(const std::string& key)
{
    if (is_valid_)
    {
        try
        {
            const nlohmann::json json_data = readConfigurationFile();
            return json_data.count(key) > 0;
        }
        catch (const std::exception& e)
        {
            LUMOS_LOG_ERROR() << "Error calling ConfigurationAgent::hasKey(" << key << ") in configuration file \""
                            << configuration_file_path_ << "\".";
            return 0;
        }
    }
    else
    {
        LUMOS_LOG_ERROR() << "Tried calling ConfigurationAgent::hasKey(" << key << ") for invalid ConfigurationAgent!";
        return false;
    }
}

nlohmann::json ConfigurationAgent::readConfigurationFile() const
{
    std::ifstream input_file(configuration_file_path_);
    nlohmann::json json_data;
    input_file >> json_data;
    return json_data;
}

void ConfigurationAgent::createEmptyConfigurationFile() const
{
    std::ofstream output_file_stream(configuration_file_path_);
    output_file_stream << "{\n}\n";
    std::cout << "Creating file at path: " << configuration_file_path_ << std::endl;
}

void ConfigurationAgent::writeToConfigurationFile(const nlohmann::json& json_data) const
{
    std::ofstream output_file(configuration_file_path_);
    output_file << std::setw(4) << json_data << std::endl;
}
