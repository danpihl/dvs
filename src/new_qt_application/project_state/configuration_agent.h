#ifndef MAIN_APPLICATION_PROJECT_STATE_CONFIGURATION_AGENT_H_
#define MAIN_APPLICATION_PROJECT_STATE_CONFIGURATION_AGENT_H_

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

// Persists the handful of app-level (not per-project) settings duoplot
// remembers across launches, at a fixed OS-specific path
// (~/Library/Preferences/duoplot/configuration.json on macOS).
//
// Typed accessors, not a generic key/value store: this used to be
// `template <typename T> T readValue(const std::string& key)` /
// `writeValue<T>(key, val)`, requiring every caller to know both the exact
// JSON key string and its type. With exactly one consumer (MainWindow) and
// two known settings, that bought nothing but footguns — a typo'd key or a
// mismatched T were both only-at-runtime failures — and cost a redundant
// hasKey()+readValue() double file-read per access. See
// ARCHITECTURE_IMPROVEMENTS.md #8. Add a new pair of typed accessors here,
// same shape as the two below, if a new setting is ever needed — the
// underlying JSON is a plain object, so this doesn't require a schema
// migration.
class ConfigurationAgent
{
public:
    ConfigurationAgent();

    bool isValid() const;

    // std::nullopt if never set (or the agent is invalid) — no dead-end
    // ambiguity between "not set" and "set to an empty string" the way a
    // bare readValue<std::string>() had.
    std::optional<std::string> getLastOpenedFile() const;
    void setLastOpenedFile(const std::string& path);

    // Owns its own default (10) and valid range (1-100) — previously
    // duplicated at the one call site that read this
    // (MainWindow::getVisualizationPeriodMs()).
    int getVisualizationPeriodMs() const;
    void setVisualizationPeriodMs(int visualization_period_ms);

private:
    void load();
    void persist();

    bool is_valid_;
    std::string configuration_file_path_;
    nlohmann::json cache_;
};

#endif  // MAIN_APPLICATION_PROJECT_STATE_CONFIGURATION_AGENT_H_
