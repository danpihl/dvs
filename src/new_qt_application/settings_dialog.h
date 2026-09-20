#ifndef NEW_QT_APPLICATION_SETTINGS_DIALOG_H_
#define NEW_QT_APPLICATION_SETTINGS_DIALOG_H_

#include <QDialog>
#include <QLineEdit>

#include <map>
#include <string>
#include <utility>

// Direct port of main_application/settings_window.{h,cpp}'s SettingsWindow —
// a generic modal dialog with one labeled text field per entry in `fields`
// (key -> {label, initial value}) plus OK/Cancel. Used by GuiWindow's
// popup-menu system to collect new-element/edit-element settings.
class SettingsDialog : public QDialog
{
public:
    using FieldsType = std::map<std::string, std::pair<std::string, std::string>>;

    SettingsDialog() = delete;
    SettingsDialog(QWidget* parent, const std::string& title, const FieldsType& fields);

    std::string getFieldString(const std::string& field_name) const;

private:
    std::map<std::string, QLineEdit*> fields_;
};

#endif  // NEW_QT_APPLICATION_SETTINGS_DIALOG_H_
