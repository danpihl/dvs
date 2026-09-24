#include "project_state/configuration_agent.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "filesystem.h"
#include "lumos/logging.h"
#include "platform_paths.h"

namespace
{
constexpr int kDefaultVisualizationPeriodMs = 10;
constexpr int kMinVisualizationPeriodMs = 1;
constexpr int kMaxVisualizationPeriodMs = 100;

const char* const kLastOpenedFileKey = "last_opened_file";
const char* const kVisualizationPeriodMsKey = "visualization_period_ms";
}  // namespace

ConfigurationAgent::ConfigurationAgent() : is_valid_{true}, cache_{nlohmann::json::object()}
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
            LUMOS_LOG_ERROR() << "Failed to create duoplot directory at \"" << duoplot_dir_path
                            << "\". Exception: " << e.what();
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
            persist();
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
            load();
            reading_successful = true;
        }
        catch (const std::exception& e)
        {
            LUMOS_LOG_WARNING() << "Error reading existing configuration file \"" << configuration_file_path_
                              << "\". Exception: " << e.what() << ". Creating a new one.";
        }

        if (!reading_successful)
        {
            cache_ = nlohmann::json::object();
            try
            {
                persist();
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

void ConfigurationAgent::load()
{
    std::ifstream input_file(configuration_file_path_);
    input_file >> cache_;
}

void ConfigurationAgent::persist()
{
    std::ofstream output_file(configuration_file_path_);
    output_file << std::setw(4) << cache_ << std::endl;
}

std::optional<std::string> ConfigurationAgent::getLastOpenedFile() const
{
    if (is_valid_ && cache_.contains(kLastOpenedFileKey))
    {
        try
        {
            return cache_[kLastOpenedFileKey].get<std::string>();
        }
        catch (const std::exception& e)
        {
            LUMOS_LOG_ERROR() << "Error reading \"" << kLastOpenedFileKey << "\" from configuration file!";
        }
    }
    return std::nullopt;
}

void ConfigurationAgent::setLastOpenedFile(const std::string& path)
{
    if (!is_valid_)
    {
        LUMOS_LOG_ERROR() << "Tried calling ConfigurationAgent::setLastOpenedFile() for invalid ConfigurationAgent!";
        return;
    }

    cache_[kLastOpenedFileKey] = path;

    try
    {
        persist();
    }
    catch (const std::exception& e)
    {
        LUMOS_LOG_ERROR() << "Failed to overwrite configuration.json at \"" << configuration_file_path_
                        << "\". Exception: " << e.what() << ". Exiting.";
        is_valid_ = false;
    }
}

int ConfigurationAgent::getVisualizationPeriodMs() const
{
    if (is_valid_ && cache_.contains(kVisualizationPeriodMsKey))
    {
        try
        {
            const int value = cache_[kVisualizationPeriodMsKey].get<int>();
            return std::max(kMinVisualizationPeriodMs, std::min(kMaxVisualizationPeriodMs, value));
        }
        catch (const std::exception& e)
        {
            LUMOS_LOG_ERROR() << "Error reading \"" << kVisualizationPeriodMsKey
                            << "\" from configuration file! Using default.";
        }
    }
    return kDefaultVisualizationPeriodMs;
}

void ConfigurationAgent::setVisualizationPeriodMs(int visualization_period_ms)
{
    if (!is_valid_)
    {
        LUMOS_LOG_ERROR() << "Tried calling ConfigurationAgent::setVisualizationPeriodMs() for invalid "
                             "ConfigurationAgent!";
        return;
    }

    cache_[kVisualizationPeriodMsKey] = visualization_period_ms;

    try
    {
        persist();
    }
    catch (const std::exception& e)
    {
        LUMOS_LOG_ERROR() << "Failed to overwrite configuration.json at \"" << configuration_file_path_
                        << "\". Exception: " << e.what() << ". Exiting.";
        is_valid_ = false;
    }
}
