#include "BrowserManager.h"
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QFileInfo>
#include <algorithm>

extern const char* ORG;
extern const char* APP;

BrowserManager::BrowserManager() { load(); }

void BrowserManager::load() {
    // Load user configs
    loadUserConfigs();

    // Discover system configs fresh
    m_systemConfigs.clear();
#ifdef Q_OS_LINUX
    discoverLinux();
#elif defined(Q_OS_WIN)
    discoverWindows();
#elif defined(Q_OS_MAC)
    discoverMac();
#endif

    // Merge: system first, then user
    m_configs = m_systemConfigs;
    for (const auto& uc : m_userConfigs) {
        m_configs.push_back(uc);
    }
}

QVector<BrowserConfig> BrowserManager::configsSortedByUsage(const ChoiceStore& store) const {
    QMap<QString,int> counts;
    for (const auto& r : store.rules()) counts[r.configId] += 1;
    QVector<BrowserConfig> sorted = m_configs;
    std::sort(sorted.begin(), sorted.end(), [&](const BrowserConfig& a, const BrowserConfig& b){
        int ca = counts.value(a.id, 0);
        int cb = counts.value(b.id, 0);
        if (ca == cb) return a.name.toLower() < b.name.toLower();
        return ca > cb;
    });
    return sorted;
}

BrowserConfig* BrowserManager::findById(const QString& id) {
    for (auto& c : m_configs) if (c.id == id) return &c;
    return nullptr;
}

void BrowserManager::loadUserConfigs() {
    m_userConfigs.clear();
    QSettings s(ORG, APP);
    int n = s.beginReadArray("userConfigs");
    for (int i = 0; i < n; ++i) {
        s.setArrayIndex(i);
        BrowserConfig c;
        c.id = s.value("id", c.id).toString();
        c.name = s.value("name").toString();
        c.exePath = s.value("exePath").toString();
        c.baseArgs = s.value("baseArgs").toStringList();
        c.iconSpec = s.value("iconSpec").toString();
        c.isSystemDiscovered = false;
        if (!c.name.isEmpty() && !c.exePath.isEmpty())
            m_userConfigs.push_back(c);
    }
    s.endArray();
}

void BrowserManager::saveUserConfigs() const {
    QSettings s(ORG, APP);
    s.beginWriteArray("userConfigs");
    for (int i = 0; i < m_userConfigs.size(); ++i) {
        s.setArrayIndex(i);
        const auto& c = m_userConfigs[i];
        s.setValue("id", c.id);
        s.setValue("name", c.name);
        s.setValue("exePath", c.exePath);
        s.setValue("baseArgs", c.baseArgs);
        s.setValue("iconSpec", c.iconSpec);
    }
    s.endArray();
}

void BrowserManager::addUserConfig(const BrowserConfig& cfg) {
    m_userConfigs.push_back(cfg);
    saveUserConfigs();
    load();
}

void BrowserManager::updateUserConfig(const BrowserConfig& cfg) {
    for (auto& c : m_userConfigs) {
        if (c.id == cfg.id) { c = cfg; break; }
    }
    saveUserConfigs();
    load();
}

void BrowserManager::removeUserConfig(const QString& id) {
    m_userConfigs.erase(std::remove_if(m_userConfigs.begin(), m_userConfigs.end(),
                        [&](const BrowserConfig& c){ return c.id == id; }),
                        m_userConfigs.end());
    saveUserConfigs();
    load();
}

// Linux discovery
QStringList BrowserManager::linuxAppDirs() {
    return {
        QDir::homePath() + "/.local/share/applications",
        "/usr/local/share/applications",
        "/usr/share/applications"
    };
}

QMap<QString, QStringList> BrowserManager::readMimeinfoCache() {
    QMap<QString, QStringList> map;
    for (const auto& dir : linuxAppDirs()) {
        QFile f(dir + "/mimeinfo.cache");
        if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        while (!f.atEnd()) {
            QByteArray line = f.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;
            int eq = line.indexOf('=');
            if (eq <= 0) continue;
            QString key = QString::fromUtf8(line.left(eq));
            QStringList vals = QString::fromUtf8(line.mid(eq+1)).split(';', Qt::SkipEmptyParts);
            map[key] = vals;
        }
    }
    return map;
}

bool BrowserManager::readDesktopEntry(const QString& id, DesktopEntry* out) {
    for (const auto& dir : linuxAppDirs()) {
        QFile f(dir + "/" + id);
        if (!f.exists()) continue;
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        DesktopEntry de; de.id = id;
        while (!f.atEnd()) {
            QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.startsWith("Name=")) de.name = line.mid(5);
            else if (line.startsWith("Exec=")) de.exec = line.mid(5);
            else if (line.startsWith("Icon=")) de.icon = line.mid(5);
        }
        if (!de.exec.isEmpty()) { *out = de; return true; }
    }
    return false;
}

BrowserConfig BrowserManager::cfgFromDesktop(const DesktopEntry& de) {
    QString execLine = de.exec;
    QString exePath;
    QStringList args;

    QRegularExpression re(R"((?:[^\s"']+|"[^"]*"|'[^']*'))");
    auto it = re.globalMatch(execLine);
    QStringList tokens;
    while (it.hasNext()) tokens << it.next().captured(0).replace("\"", "");
    if (!tokens.isEmpty()) {
        exePath = tokens.takeFirst();
        for (const QString& t : tokens) {
            if (t.startsWith('%')) continue; // skip placeholders
            args << t;
        }
    }

    BrowserConfig c;
    c.name = de.name.isEmpty() ? de.id : de.name;
    c.exePath = exePath;
    c.baseArgs = args;
    c.iconSpec = de.icon;
    c.isSystemDiscovered = true;
    return c;
}

void BrowserManager::discoverLinux() {
    auto cache = readMimeinfoCache();
    QStringList ids = cache.value("x-scheme-handler/http");
    for (const auto& id : cache.value("x-scheme-handler/https"))
        if (!ids.contains(id)) ids << id;

    for (const auto& id : ids) {
        DesktopEntry de;
        if (readDesktopEntry(id, &de)) {
            auto cfg = cfgFromDesktop(de);
            if (!cfg.exePath.isEmpty())
                m_systemConfigs.push_back(cfg);
        }
    }
}

// Windows discovery
QVector<BrowserManager::WinRegBrowser> BrowserManager::enumerateWindowsRegistry() {
    QVector<WinRegBrowser> out;

    auto regRoots = QVector<QString>{
        "HKEY_CURRENT_USER\\Software\\Clients\\StartMenuInternet",
        "HKEY_LOCAL_MACHINE\\Software\\Clients\\StartMenuInternet"
    };

    auto readOpenCommand = [](const QString& baseKey) -> QString {
        QSettings cmd(baseKey + "\\shell\\open\\command", QSettings::NativeFormat);
        return cmd.value(".").toString();
    };
    auto readDefaultIcon = [](const QString& baseKey) -> QString {
        QSettings icon(baseKey + "\\DefaultIcon", QSettings::NativeFormat);
        return icon.value(".").toString();
    };
    auto parseCommand = [](const QString& cmd) -> QPair<QString, QStringList> {
        QString s = cmd.trimmed();
        QString exe;
        QStringList args;
        if (s.startsWith('"')) {
            int end = s.indexOf('"', 1);
            if (end > 1) {
                exe = s.mid(1, end - 1);
                s = s.mid(end + 1).trimmed();
            } else {
                exe = s; s.clear();
            }
        } else {
            int sp = s.indexOf(' ');
            exe = sp > 0 ? s.left(sp) : s;
            s = sp > 0 ? s.mid(sp + 1) : QString();
        }
        for (const auto& token : s.split(' ', Qt::SkipEmptyParts)) {
            QString t = token;
            if (t.contains("%1") || t.contains("%l") || t.contains("%L")) continue;
            args << t.replace("\"", "");
        }
        return {exe, args};
    };

    for (const auto& rootKey : regRoots) {
        QSettings root(rootKey, QSettings::NativeFormat);
        QStringList browsers = root.childGroups();
        for (const auto& bkey : browsers) {
            QString baseKey = rootKey + "\\" + bkey;
            QSettings b(baseKey, QSettings::NativeFormat);
            QString name = b.value(".").toString();
            QString cmd = readOpenCommand(baseKey);
            if (cmd.isEmpty()) continue;
            auto parsed = parseCommand(cmd);
            if (parsed.first.isEmpty()) continue;

            WinRegBrowser br;
            br.name = name.isEmpty() ? bkey : name;
            br.exe = parsed.first;
            br.args = parsed.second;
            br.iconPath = readDefaultIcon(baseKey);
            out.push_back(br);
        }
    }
    return out;
}

void BrowserManager::discoverWindows() {
    auto regs = enumerateWindowsRegistry();
    for (const auto& rb : regs) {
        BrowserConfig c;
        c.name = rb.name;
        c.exePath = rb.exe;
        c.baseArgs = rb.args;
        c.iconSpec = rb.iconPath.isEmpty() ? rb.exe : rb.iconPath;
        c.isSystemDiscovered = true;
        if (!c.exePath.isEmpty())
            m_systemConfigs.push_back(c);
    }
}

// macOS discovery (best-effort)
#ifdef Q_OS_MAC
#include <QDir>
#endif

QVector<BrowserConfig> BrowserManager::discoverMacApps() {
    QVector<BrowserConfig> out;
#ifdef Q_OS_MAC
    QStringList dirs = {"/Applications", QDir::homePath() + "/Applications"};
    QStringList candidates = {"Firefox.app", "Google Chrome.app", "Chromium.app", "Safari.app",
                              "Brave Browser.app", "Vivaldi.app", "Opera.app"};
    for (const auto& d : dirs) {
        for (const auto& appName : candidates) {
            QString appPath = d + "/" + appName;
            if (!QFileInfo(appPath).exists()) continue;
            QString exe = appPath + "/Contents/MacOS/" + QFileInfo(appName).baseName();
            if (!QFileInfo(exe).exists()) continue;

            BrowserConfig c;
            c.name = QFileInfo(appName).baseName();
            c.exePath = exe;
            c.iconSpec = appPath;
            c.isSystemDiscovered = true;
            out.push_back(c);
        }
    }
#endif
    return out;
}

void BrowserManager::discoverMac() {
    for (const auto& c : discoverMacApps()) {
        m_systemConfigs.push_back(c);
    }
}
