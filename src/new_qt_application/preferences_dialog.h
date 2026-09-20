#ifndef NEW_QT_APPLICATION_PREFERENCES_DIALOG_H_
#define NEW_QT_APPLICATION_PREFERENCES_DIALOG_H_

#include <QDialog>
#include <QSpinBox>

// New functionality, not a port: wx's own "Preferences"
// (main_application/main_window.cpp:974's preferences()) is a non-functional
// stub that prints "Preferences!" to stdout — the tray icon's Preferences
// menu item that would trigger it is even commented out, so it was never
// reachable in practice. There's no SettingsHandler class or real settings
// dialog anywhere in main_application to mimic. This is a genuine, if
// small, preferences window for the one ConfigurationAgent-backed tunable
// that actually exists: visualization_period_ms (main_window.cpp:152-159
// in wx, read once at startup and never exposed to the user otherwise).
class PreferencesDialog : public QDialog
{
public:
    PreferencesDialog() = delete;
    PreferencesDialog(QWidget* parent, const int current_visualization_period_ms);

    int getVisualizationPeriodMs() const;

private:
    QSpinBox* visualization_period_spinbox_;
};

#endif  // NEW_QT_APPLICATION_PREFERENCES_DIALOG_H_
