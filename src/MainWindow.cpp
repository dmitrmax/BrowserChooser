#include "MainWindow.h"
#include "ChoiceEditorDialog.h"
#include "ConfigDialog.h"
#include <QCoreApplication>
#include <QKeyEvent>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QSettings>

extern const char* ORG;
extern const char* APP;

MainWindow::MainWindow(BrowserManager* mgr, ChoiceStore* store,
                       const QString& urlParam, QWidget* parent)
    : QMainWindow(parent), m_mgr(mgr), m_store(store), m_url(urlParam)
{
    auto* central = new QWidget(this);
    auto* v = new QVBoxLayout(central);

    m_urlLabel = new QLabel(tr("URL:"), this);

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setReadOnly(true);
    m_urlEdit->setText(m_url);
    m_urlEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_urlEdit->home(false);
    m_urlLabel->setBuddy(m_urlEdit);
    v->addWidget(m_urlLabel);
    v->addWidget(m_urlEdit);

    m_list = new QListWidget(this);
    populateList();
    v->addWidget(m_list);

    m_rememberCheck = new QCheckBox(tr("Remember this choice for domain"), this);
    m_baseDomainCheck = new QCheckBox(tr("Use base domain (example.com)"), this);
    m_baseDomainCheck->setChecked(true);
    m_rememberCheck->setChecked(true);
    if (m_url.isEmpty()) {
        m_rememberCheck->setEnabled(false);
        m_baseDomainCheck->setEnabled(false);
    }
    v->addWidget(m_rememberCheck);
    v->addWidget(m_baseDomainCheck);

    // Preferences toggles
    m_checkDefaultOnStartup = new QCheckBox(tr("Check default browser on startup"), this);
    m_ignoreDefaultBrowserEnv = new QCheckBox(tr("Ignore DEFAULT_BROWSER override"), this);
    m_unwrapSlackUrls = new QCheckBox(tr("Unwrap Slack URLs"), this);

    QSettings s(ORG, APP);
    m_checkDefaultOnStartup->setChecked(s.value("General/checkDefaultOnStartup", true).toBool());
    m_ignoreDefaultBrowserEnv->setChecked(s.value("General/ignoreDefaultBrowserEnv", false).toBool());
    m_unwrapSlackUrls->setChecked(s.value("General/unwrapSlackUrls", true).toBool());

    connect(m_checkDefaultOnStartup, &QCheckBox::toggled, this, [&](bool on){
        QSettings s(ORG, APP);
        s.setValue("General/checkDefaultOnStartup", on);
    });
    connect(m_ignoreDefaultBrowserEnv, &QCheckBox::toggled, this, [&](bool on){
        QSettings s(ORG, APP);
        s.setValue("General/ignoreDefaultBrowserEnv", on);
    });
    connect(m_unwrapSlackUrls, &QCheckBox::toggled, this, [&](bool on){
        QSettings s(ORG, APP);
        s.setValue("General/unwrapSlackUrls", on);
    });


    v->addWidget(m_checkDefaultOnStartup);
    v->addWidget(m_ignoreDefaultBrowserEnv);
    v->addWidget(m_unwrapSlackUrls);

    auto* btnRow = new QHBoxLayout();
    auto* openBtn = new QPushButton(tr("Open"), this);
    auto* editChoicesBtn = new QPushButton(tr("Edit remembered choices"), this);
    auto* newCfgBtn = new QPushButton(tr("New config"), this);
    auto* cloneCfgBtn = new QPushButton(tr("Clone config"), this);
    m_editCfgBtn = new QPushButton(tr("Edit config"), this);
    m_delCfgBtn = new QPushButton(tr("Delete config"), this);

    openBtn->setDefault(true);

    btnRow->addWidget(openBtn);
    btnRow->addWidget(editChoicesBtn);
    btnRow->addStretch();
    btnRow->addWidget(newCfgBtn);
    btnRow->addWidget(m_editCfgBtn);
    btnRow->addWidget(m_delCfgBtn);
    btnRow->addWidget(cloneCfgBtn);
    v->addLayout(btnRow);

    setCentralWidget(central);
    setWindowTitle(tr("Browser Chooser"));

    connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenClicked);
    connect(editChoicesBtn, &QPushButton::clicked, this, &MainWindow::onEditChoicesClicked);
    connect(newCfgBtn, &QPushButton::clicked, this, &MainWindow::onNewConfigClicked);
    connect(cloneCfgBtn, &QPushButton::clicked, this, &MainWindow::onCloneConfigClicked);
    connect(m_editCfgBtn, &QPushButton::clicked, this, &MainWindow::onEditConfigClicked);
    connect(m_delCfgBtn, &QPushButton::clicked, this, &MainWindow::onDeleteConfigClicked);
    connect(m_list, &QListWidget::currentItemChanged, this, &MainWindow::onCurrentItemChanged);

    if (m_checkDefaultOnStartup->isChecked()) {
        checkAndOfferDefault();
    }
}

void MainWindow::populateList() {
    m_list->clear();
    auto sorted = m_mgr->configsSortedByUsage(*m_store);
    for (const auto& c : sorted) {
        auto* item = new QListWidgetItem(c.name, m_list);
        item->setData(Qt::UserRole, c.id);
        item->setIcon(c.resolveIcon());
    }
}

QString MainWindow::currentDomainKey(DomainMatchMode* outMode) const {
    if (m_url.isEmpty()) return QString();
    QUrl url(m_url);
    QString host = url.host();
    if (host.isEmpty()) return QString();
    DomainMatchMode mode = m_baseDomainCheck->isChecked() ? DomainMatchMode::BaseDomain : DomainMatchMode::FullHost;
    if (outMode) *outMode = mode;

    if (mode == DomainMatchMode::FullHost) return host;
    QStringList parts = host.split('.');
    if (parts.size() >= 2) return parts[parts.size()-2] + "." + parts[parts.size()-1];
    return host;
}

void MainWindow::onOpenClicked() {
    auto* item = m_list->currentItem();
    if (!item) {
        QMessageBox::warning(this, tr("No selection"), tr("Please select a browser configuration."));
        return;
    }
    QString id = item->data(Qt::UserRole).toString();
    BrowserConfig* cfg = m_mgr->findById(id);
    if (!cfg) return;

    if (m_rememberCheck->isChecked() && !m_url.isEmpty()) {
        DomainMatchMode mode;
        QString key = currentDomainKey(&mode);
        if (!key.isEmpty()) {
            ChoiceRule r{key, mode, cfg->id, 0};
            m_store->setRule(r);
        }
    }
    launchConfig(*cfg, m_url);
}

void MainWindow::launchConfig(const BrowserConfig& cfg, const QString& url) {
    QStringList args = cfg.buildArgsForUrl(url);
    bool ok = QProcess::startDetached(cfg.exePath, args);
    if (!ok) {
        QMessageBox::critical(this, tr("Launch failed"),
                              tr("Failed to start %1").arg(cfg.name));
        return;
    }
    // Terminate the whole app after launching
    QCoreApplication::exit(0);
}

void MainWindow::onEditChoicesClicked() {
    ChoiceEditorDialog dlg(m_mgr, m_store, this);
    dlg.exec();
    populateList(); // usage counts may change
}

void MainWindow::onNewConfigClicked() {
    ConfigDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        BrowserConfig cfg = dlg.getConfig();
        if (cfg.name.isEmpty() || cfg.exePath.isEmpty()) return;
        m_mgr->addUserConfig(cfg);
        populateList();
    }
}

void MainWindow::onEditConfigClicked() {
    auto* item = m_list->currentItem();
    if (!item) return;
    QString id = item->data(Qt::UserRole).toString();
    BrowserConfig* cfg = m_mgr->findById(id);
    if (!cfg || cfg->isSystemDiscovered) {
        QMessageBox::information(this, tr("Info"), tr("System-discovered configs are read-only. Clone to customize."));
        return;
    }
    ConfigDialog dlg(this);
    dlg.setConfig(*cfg);
    if (dlg.exec() == QDialog::Accepted) {
        BrowserConfig edited = dlg.getConfig();
        edited.id = cfg->id; // preserve ID
        m_mgr->updateUserConfig(edited);
        populateList();
    }
}

void MainWindow::onDeleteConfigClicked() {
    auto* item = m_list->currentItem();
    if (!item) return;
    QString id = item->data(Qt::UserRole).toString();
    BrowserConfig* cfg = m_mgr->findById(id);
    if (!cfg || cfg->isSystemDiscovered) {
        QMessageBox::information(this, tr("Info"), tr("You can only delete custom (user) configurations."));
        return;
    }
    auto ret = QMessageBox::question(this, tr("Delete config"), tr("Delete selected configuration?"));
    if (ret == QMessageBox::Yes) {
        m_mgr->removeUserConfig(id);
        populateList();
    }
}

void MainWindow::onCloneConfigClicked() {
    auto* item = m_list->currentItem();
    if (!item) return;
    QString id = item->data(Qt::UserRole).toString();
    BrowserConfig* cfg = m_mgr->findById(id);
    if (!cfg) return;

    BrowserConfig clone = *cfg;
    clone.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    clone.name = tr("Copy of %1").arg(cfg->name);
    clone.isSystemDiscovered = false;

    ConfigDialog dlg(this);
    dlg.setConfig(clone);
    if (dlg.exec() == QDialog::Accepted) {
        BrowserConfig finalCfg = dlg.getConfig();
        if (finalCfg.name.isEmpty() || finalCfg.exePath.isEmpty()) return;
        m_mgr->addUserConfig(finalCfg);
        populateList();
    }
}

void MainWindow::onCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current) return;

    QString id = current->data(Qt::UserRole).toString();
    BrowserConfig* cfg = m_mgr->findById(id);
    if (!cfg) return;

    bool discovered = cfg->isSystemDiscovered;

    m_editCfgBtn->setEnabled(!discovered);
    m_delCfgBtn->setEnabled(!discovered);

    if (discovered) {
        m_editCfgBtn->setToolTip(tr("Discovered browsers cannot be edited"));
        m_delCfgBtn->setToolTip(tr("Discovered browsers cannot be deleted"));
    } else {
        m_editCfgBtn->setToolTip(QString());
        m_delCfgBtn->setToolTip(QString());
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        // Close the window without launching browser
        close();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

// Default browser checking

QString MainWindow::desktopFileId() const {
    // Must match your installed .desktop filename
    return "org.example.BrowserChooser.desktop";
}

QString MainWindow::defaultHandlerForScheme(const QString& scheme) const {
#ifdef Q_OS_LINUX
    QString key = QString("x-scheme-handler/") + scheme;
    QStringList paths = {
        QDir::homePath() + "/.config/mimeapps.list",
        QDir::homePath() + "/.local/share/applications/mimeapps.list",
        "/usr/local/share/applications/mimeapps.list",
        "/usr/share/applications/mimeapps.list"
    };
    for (const auto& p : paths) {
        QFile f(p);
        if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        bool inDefault = false;
        while (!f.atEnd()) {
            QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.startsWith('[')) inDefault = (line == "[Default Applications]");
            else if (inDefault && line.startsWith(key + "=")) {
                QString val = line.section('=', 1);
                return val.split(';', Qt::SkipEmptyParts).value(0);
            }
        }
    }
#endif
    return QString();
}

#if defined(Q_OS_MAC)
extern "C" { void setDefaultURLHandlerForHttpHttpsMac(); }
#endif

void MainWindow::checkAndOfferDefault() {
#ifdef Q_OS_LINUX
    auto currentHttp = defaultHandlerForScheme("http");
    auto currentHttps = defaultHandlerForScheme("https");
    if (currentHttp != desktopFileId() || currentHttps != desktopFileId()) {
        auto ret = QMessageBox::question(this, tr("Set default"),
            tr("Browser Chooser is not the default browser. Make it default?"));
        if (ret == QMessageBox::Yes) {
            QProcess::execute("xdg-mime", {"default", desktopFileId(), "x-scheme-handler/http"});
            QProcess::execute("xdg-mime", {"default", desktopFileId(), "x-scheme-handler/https"});
            QMessageBox::information(this, tr("Done"), tr("Default browser updated."));
        }
    }
#elif defined(Q_OS_WIN)
    auto ret = QMessageBox::question(this, tr("Set default"),
        tr("Make Browser Chooser your default browser? You'll confirm in Windows settings."));
    if (ret == QMessageBox::Yes) {
        QProcess::startDetached("ms-settings:defaultapps");
        QMessageBox::information(this, tr("Tip"),
            tr("Find 'Browser Chooser' under Default apps for Web browser."));
    }
#elif defined(Q_OS_MAC)
    auto ret = QMessageBox::question(this, tr("Set default"),
        tr("Make Browser Chooser your default browser for http/https?"));
    if (ret == QMessageBox::Yes) {
        setDefaultURLHandlerForHttpHttpsMac();
        QMessageBox::information(this, tr("Done"), tr("Default browser updated."));
    }
#endif
}
