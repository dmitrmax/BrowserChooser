#pragma once
#include <QString>
#include <QStringList>
#include <QIcon>
#include <QUuid>
#include <QFileInfo>

struct BrowserConfig {
    QString id;            // Stable UUID
    QString name;          // Display name
    QString exePath;       // Executable path
    QStringList baseArgs;  // Args excluding URL
    QString iconSpec;      // Theme icon name or absolute path (Linux), path/exe (Windows/macOS)
    bool isSystemDiscovered = false; // Read-only system config

    BrowserConfig() : id(QUuid::createUuid().toString(QUuid::WithoutBraces)) {}

    QStringList buildArgsForUrl(const QString& url) const {
        QStringList args = baseArgs;
        if (!url.isEmpty()) args << url;
        return args;
    }

    QIcon resolveIcon() const {
#ifdef Q_OS_LINUX
        if (!iconSpec.isEmpty()) {
            if (QFileInfo(iconSpec).isAbsolute()) {
                QIcon ic(iconSpec);
                if (!ic.isNull()) return ic;
            } else {
                QIcon ic = QIcon::fromTheme(iconSpec);
                if (!ic.isNull()) return ic;
            }
        }
        QIcon generic = QIcon::fromTheme("applications-internet");
        if (!generic.isNull()) return generic;
        return QIcon(":/icons/default-browser.png");
#elif defined(Q_OS_WIN)
        if (!iconSpec.isEmpty() && QFileInfo(iconSpec).exists()) {
            QIcon ic(iconSpec);
            if (!ic.isNull()) return ic;
        }
        if (!exePath.isEmpty() && QFileInfo(exePath).exists()) {
            QIcon ic(exePath);
            if (!ic.isNull()) return ic;
        }
        return QIcon(":/icons/default-browser.png");
#elif defined(Q_OS_MAC)
        // macOS: QIcon can load bundle icons if you pass the .app path
        if (!iconSpec.isEmpty() && QFileInfo(iconSpec).exists()) {
            QIcon ic(iconSpec);
            if (!ic.isNull()) return ic;
        }
        if (!exePath.isEmpty() && QFileInfo(exePath).exists()) {
            QIcon ic(exePath);
            if (!ic.isNull()) return ic;
        }
        return QIcon(":/icons/default-browser.png");
#else
        return QIcon(":/icons/default-browser.png");
#endif
    }
};
