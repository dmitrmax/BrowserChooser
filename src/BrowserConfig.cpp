#include "BrowserConfig.h"

void BrowserConfig::save(QSettings& s) const
{
    s.setValue("name", name_);
    s.setValue("exePath", exePath_);
    s.setValue("baseArgs", baseArgs_);
    s.setValue("iconSpec", iconSpec_);
}

BrowserConfig BrowserConfig::load(QSettings& s)
{
    QString name = s.value("name").toString();
    QString exe  = s.value("exePath").toString();
    QStringList args = s.value("baseArgs").toStringList();
    QString icon = s.value("iconSpec").toString();

    return BrowserConfig(name, exe, args, icon, false);
}
