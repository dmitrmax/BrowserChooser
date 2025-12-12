#include <QApplication>
#include <QStringList>
#include <QCommandLineParser>
#include <QTranslator>
#include <QLocale>
#include <QSettings>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QUrl>
#include <QFileOpenEvent>
#include <QDebug>

#include "MainWindow.h"
#include "BrowserManager.h"
#include "ChoiceStore.h"
#include "UrlUtils.h"

const char* ORG = "com.github.dmitrmax";
const char* APP = "BrowserChooser";

// Environment override: DEFAULT_BROWSER
static bool forwardToDefaultBrowserEnv(int argc, char* argv[]) {
    QByteArray env = qgetenv("DEFAULT_BROWSER");
    if (env.isEmpty()) return false;

    QString path = QString::fromLocal8Bit(env);
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isExecutable()) return false;

    // Compare with our own binary
    QString self = QCoreApplication::applicationFilePath();
    if (QFileInfo(self).canonicalFilePath() == fi.canonicalFilePath()) {
        return false; // same binary, ignore
    }

    // Build argument list (skip argv[0], forward rest)
    QStringList args;
    for (int i = 1; i < argc; ++i) {
        args << QString::fromLocal8Bit(argv[i]);
    }

    bool ok = QProcess::startDetached(path, args);
    return ok;
}

// Custom QApplication to intercept QFileOpenEvent on macOS
class BrowserChooserApp : public QApplication {
public:
    BrowserChooserApp(int &argc, char **argv)
        : QApplication(argc, argv) {}

    QString interceptedUrl;

protected:
    bool event(QEvent *e) override {
#ifdef Q_OS_MAC
        if (e->type() == QEvent::FileOpen) {
            QFileOpenEvent *foe = static_cast<QFileOpenEvent *>(e);
            interceptedUrl = foe->url().toString();
            qDebug() << "Intercepted URL from macOS event:" << interceptedUrl;
            return true;
        }
#endif
        return QApplication::event(e);
    }
};

int main(int argc, char *argv[]) {
    if (forwardToDefaultBrowserEnv(argc, argv))
        return 0;

    BrowserChooserApp app(argc, argv);
    QApplication::setOrganizationName(ORG);
    QApplication::setApplicationName(APP);

    QTranslator translator;
    const QString locale = QLocale::system().name(); // e.g. ru_RU
    bool loaded = false;
    if ((loaded = translator.load(QString(":/i18n/BrowserChooser_%1.qm").arg(locale))) ||
        (loaded = translator.load(QString("translations/BrowserChooser_%1.qm").arg(locale)))) {
        app.installTranslator(&translator);
    }

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption urlOpt({"u","url"}, QObject::tr("URL to open"), "url");
    parser.addOption(urlOpt);
    parser.process(app);

    QString url = parser.value(urlOpt);
    if (url.isEmpty()) {
        const QStringList pos = parser.positionalArguments();
        if (!pos.isEmpty()) url = pos.first();
    }

#ifdef Q_OS_MAC
    // If no URL from command line, check intercepted event URL
    if (url.isEmpty() && !app.interceptedUrl.isEmpty()) {
        url = app.interceptedUrl;
    }
#endif

    BrowserManager mgr;
    ChoiceStore store;

    if (!url.isEmpty()) {
        QSettings s(ORG, APP);
        bool slackFilter = s.value("General/unwrapSlackUrls", true).toBool();
        url = unwrapSlackUrl(url, slackFilter);
    }

    if (!url.isEmpty()) {
        QString host = QUrl(url).host();
        QString cfgId = store.matchConfigIdForHost(host);
        if (!cfgId.isEmpty()) {
            BrowserConfig* cfg = mgr.findById(cfgId);
            if (cfg) {
                QStringList args = cfg->buildArgsForUrl(url);
                if (QProcess::startDetached(cfg->exePath, args)) {
                    return 0; // launched, exit immediately
                }
            }
        }
    }

    // Only show chooser if no remembered rule applied
    MainWindow w(&mgr, &store, url);
    w.resize(760, 520);
    w.show();
    return app.exec();
}
