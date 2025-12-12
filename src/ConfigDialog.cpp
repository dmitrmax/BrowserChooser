#include "ConfigDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QLabel>
#include <QProcess>

ConfigDialog::ConfigDialog(QWidget* parent) : QDialog(parent) {
    auto* v = new QVBoxLayout(this);

    m_name = new QLineEdit(this);
    m_exe  = new QLineEdit(this);
    m_args = new QLineEdit(this);
    m_icon = new QLineEdit(this);
    auto* pickIcon = new QPushButton(tr("Browse icon..."), this);

    v->addWidget(new QLabel(tr("Display name:"), this));
    v->addWidget(m_name);
    v->addWidget(new QLabel(tr("Executable path:"), this));
    v->addWidget(m_exe);
    v->addWidget(new QLabel(tr("Base arguments (without URL):"), this));
    v->addWidget(m_args);
    v->addWidget(new QLabel(tr("Icon (theme name or absolute path):"), this));
    auto* h = new QHBoxLayout();
    h->addWidget(m_icon);
    h->addWidget(pickIcon);
    v->addLayout(h);

    auto* btns = new QHBoxLayout();
    auto* ok = new QPushButton(tr("OK"), this);
    auto* cancel = new QPushButton(tr("Cancel"), this);
    btns->addStretch(); btns->addWidget(ok); btns->addWidget(cancel);
    v->addLayout(btns);

    connect(pickIcon, &QPushButton::clicked, this, &ConfigDialog::onPickIcon);
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    setWindowTitle(tr("Browser configuration"));
    resize(520, 320);
}

void ConfigDialog::setConfig(const BrowserConfig& cfg) {
    m_name->setText(cfg.name);
    m_exe->setText(cfg.exePath);
    m_args->setText(cfg.baseArgs.join(' '));
    m_icon->setText(cfg.iconSpec);
}

BrowserConfig ConfigDialog::getConfig() const {
    BrowserConfig cfg;
    cfg.name = m_name->text();
    cfg.exePath = m_exe->text();

    QString argLine = m_args->text().trimmed();
    if (!argLine.isEmpty()) {
        cfg.baseArgs = QProcess::splitCommand(argLine);
    }

    cfg.iconSpec = m_icon->text();
    cfg.isSystemDiscovered = false;
    return cfg;
}

void ConfigDialog::onPickIcon() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select icon"), QString(),
        tr("Images (*.png *.ico *.svg *.xpm);;All files (*)"));
    if (!path.isEmpty()) m_icon->setText(path);
}
