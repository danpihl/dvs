#ifndef MAIN_APPLICATION_PROJECT_STATE_CONFIGURATION_AGENT_H_
#define MAIN_APPLICATION_PROJECT_STATE_CONFIGURATION_AGENT_H_

#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <string>

#include "lumos/logging.h"
#include "filesystem.h"

struct AppPreferences
{
    std::string last_opened_file;
    bool open_main_window_on_start;
};

class ConfigurationAgent
{
public:
    ~ConfigurationAgent();
    ConfigurationAgent();

    bool isValid() const;
    bool hasKey(const std::string& key);

    template <typename T> T readValue(const std::string& key)
    {
        if (is_valid_)
        {
            nlohmann::json json_data;
            T data;
            try
            {
                json_data = readConfigurationFile();
            }
            catch (const std::exception& e)
            {
                is_valid_ = false;
                LUMOS_LOG_ERROR() << "Error reading configuration file \"" << configuration_file_path_
                                << "\"! Returning empty value.";
                return data;
            }

            try
            {
                data = json_data[key].get<T>();
            }
            catch (const std::exception& e)
            {
                LUMOS_LOG_ERROR() << "Error reading value for key " << key << "! Returning empty value.";
            }
            return data;
        }
        else
        {
            LUMOS_LOG_ERROR() << "Tried calling ConfigurationAgent::readValue(" << key
                            << ") for invalid ConfigurationAgent!";
            T data = 0;
            return data;
        }
    }

    template <typename T> void writeValue(const std::string& key, const T& val)
    {
        if (is_valid_)
        {
            nlohmann::json json_data = readConfigurationFile();
            json_data[key] = val;

            try
            {
                writeToConfigurationFile(json_data);
            }
            catch (const std::exception& e)
            {
                LUMOS_LOG_ERROR() << "Failed to overwrite configuration.json at \"" << configuration_file_path_
                                << "\". Exception: " << e.what() << ". Exiting.";
                is_valid_ = false;
                return;
            }
        }
        else
        {
            LUMOS_LOG_ERROR() << "Tried calling ConfigurationAgent::writeValue(" << key
                            << ") for invalid ConfigurationAgent!";
        }
    }

private:
    nlohmann::json readConfigurationFile() const;
    void createEmptyConfigurationFile() const;
    void writeToConfigurationFile(const nlohmann::json& json_data) const;

    bool is_valid_;
    std::string configuration_file_path_;
};

#endif  // MAIN_APPLICATION_PROJECT_STATE_CONFIGURATION_AGENT_H_
