#include "preferences_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(QWidget* parent, const int current_visualization_period_ms)
    : QDialog(parent)
{
    setWindowTitle("Preferences");

    QVBoxLayout* layout = new QVBoxLayout(this);
    QFormLayout* form_layout = new QFormLayout();

    visualization_period_spinbox_ = new QSpinBox(this);
    visualization_period_spinbox_->setRange(1, 100);
    visualization_period_spinbox_->setValue(current_visualization_period_ms);
    visualization_period_spinbox_->setSuffix(" ms");
    form_layout->addRow("Visualization period", visualization_period_spinbox_);

    layout->addLayout(form_layout);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

int PreferencesDialog::getVisualizationPeriodMs() const
{
    return visualization_period_spinbox_->value();
}
