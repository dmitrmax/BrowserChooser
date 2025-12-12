#pragma once
#include "BrowserConfig.h"
#include "ChoiceStore.h"
#include <QVector>
#include <QMap>

class BrowserManager {
public:
    BrowserManager();

    void load(); // Every launch: discover system configs + load user configs

    QVector<BrowserConfig> configsSortedByUsage(const ChoiceStore& store) const;
    BrowserConfig* findById(const QString& id);

    // User config management
    void addUserConfig(const BrowserConfig& cfg);
    void updateUserConfig(const BrowserConfig& cfg);
    void removeUserConfig(const QString& id);
    void saveUserConfigs() const;

private:
    QVector<BrowserConfig> m_configs;
    QVector<BrowserConfig> m_systemConfigs;
    QVector<BrowserConfig> m_userConfigs;

    // Discovery
    void discoverLinux();
    void discoverWindows();
    void discoverMac();

    // Persistence
    void loadUserConfigs();

    // Linux helpers
    static QStringList linuxAppDirs();
    static QMap<QString, QStringList> readMimeinfoCache();
    struct DesktopEntry { QString id, name, exec, icon; };
    static bool readDesktopEntry(const QString& id, DesktopEntry* out);
    static BrowserConfig cfgFromDesktop(const DesktopEntry& de);

    // Windows helpers
    struct WinRegBrowser { QString name; QString exe; QStringList args; QString iconPath; };
    static QVector<WinRegBrowser> enumerateWindowsRegistry();

    // macOS helpers
    static QVector<BrowserConfig> discoverMacApps();
};
