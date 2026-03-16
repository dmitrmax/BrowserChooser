#pragma once
#include <QString>
#include <QStringList>
#include <QIcon>
#include <QUuid>
#include <QFileInfo>
#include <QSettings>

class BrowserConfig
{
public:
    BrowserConfig(
        QString name,
        QString exePath,
        QStringList baseArgs = {},
        QString iconSpec = {},
        bool isSystemDiscovered = false)
        : name_(std::move(name)),
          exePath_(std::move(exePath)),
          baseArgs_(std::move(baseArgs)),
          iconSpec_(std::move(iconSpec)),
          isSystemDiscovered_(isSystemDiscovered_),
          id_(makeId(exePath_, baseArgs_))
    {}

    const QString& id() const { return id_; }
    const QString& name() const { return name_; }
    const QString& exePath() const { return exePath_; }
    const QString& iconSpec() const { return iconSpec_; }
    const QStringList& baseArgs() const { return baseArgs_; }
    bool isSystemDiscovered() const { return isSystemDiscovered_; }

    void setId(const QString& id) { id_ = id; }

    QStringList buildArgsForUrl(const QString& url) const
    {
        QStringList args = baseArgs_;
        if (!url.isEmpty())
            args << url;
        return args;
    }

    void save(QSettings& s) const;
    static BrowserConfig load(QSettings& s);

private:
    QString name_;
    QString exePath_;
    QStringList baseArgs_;
    QString iconSpec_;
    bool isSystemDiscovered_ = false; // Read-only system config
    QString id_;

    static QString makeId(const QString& exePath, const QStringList& args)
    {
        static const QUuid ns("{8b7c6f72-7e6e-4b1e-9a6f-2c9f4e4c7a2d}");

        QByteArray key;
        key += exePath.toUtf8();
        key += '\0';

        for (const QString& arg : args) {
            key += arg.toUtf8();
            key += '\0';
        }

        QUuid uuid = QUuid::createUuidV5(ns, key);

        return uuid.toString(QUuid::WithoutBraces);
    }

public:
    QIcon resolveIcon() const {
#ifdef Q_OS_LINUX
        if (!iconSpec_.isEmpty()) {
            if (QFileInfo(iconSpec_).isAbsolute()) {
                QIcon ic(iconSpec_);
                if (!ic.isNull()) return ic;
            } else {
                QIcon ic = QIcon::fromTheme(iconSpec_);
                if (!ic.isNull()) return ic;
            }
        }
        QIcon generic = QIcon::fromTheme("applications-internet");
        if (!generic.isNull()) return generic;
        return QIcon(":/icons/default-browser.png");
#elif defined(Q_OS_WIN)
        if (!iconSpec_.isEmpty() && QFileInfo(iconSpec_).exists()) {
            QIcon ic(iconSpec_);
            if (!ic.isNull()) return ic;
        }
        if (!exePath.isEmpty() && QFileInfo(exePath).exists()) {
            QIcon ic(exePath);
            if (!ic.isNull()) return ic;
        }
        return QIcon(":/icons/default-browser.png");
#elif defined(Q_OS_MAC)
        // macOS: QIcon can load bundle icons if you pass the .app path
        if (!iconSpec_.isEmpty() && QFileInfo(iconSpec_).exists()) {
            QIcon ic(iconSpec_);
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
