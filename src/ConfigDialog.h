#pragma once
#include <QDialog>
#include "BrowserConfig.h"

class QLineEdit;

class ConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConfigDialog(QWidget* parent = nullptr);
    void setConfig(const BrowserConfig& cfg);
    BrowserConfig getConfig() const;

private slots:
    void onPickIcon();

private:
    QLineEdit* m_name;
    QLineEdit* m_exe;
    QLineEdit* m_args;
    QLineEdit* m_icon;
};
