#include "settings_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent, const std::string& title, const FieldsType& fields)
    : QDialog(parent)
{
    setWindowTitle(QString::fromStdString(title));

    QVBoxLayout* layout = new QVBoxLayout(this);
    QFormLayout* form_layout = new QFormLayout();

    for (const auto& p : fields)
    {
        const std::string& key_name = p.first;
        const std::string& field_description = p.second.first;
        const std::string& field_init_value = p.second.second;

        QLineEdit* field = new QLineEdit(QString::fromStdString(field_init_value), this);
        form_layout->addRow(QString::fromStdString(field_description), field);
        fields_[key_name] = field;
    }

    layout->addLayout(form_layout);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

std::string SettingsDialog::getFieldString(const std::string& field_name) const
{
    return fields_.at(field_name)->text().toStdString();
}
