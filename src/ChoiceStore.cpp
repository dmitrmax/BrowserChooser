#include "ChoiceStore.h"
#include <QDateTime>

extern const char* ORG;
extern const char* APP;

ChoiceStore::ChoiceStore() { load(); }

QString ChoiceStore::matchConfigIdForHost(const QString& host) const {
    if (m_rules.contains(host)) return m_rules.value(host).configId;
    QString base = baseDomainFromHost(host);
    if (!base.isEmpty() && m_rules.contains(base)) return m_rules.value(base).configId;
    return QString();
}

void ChoiceStore::setRule(const ChoiceRule& rule) {
    ChoiceRule r = rule;
    if (r.timestampMs == 0) r.timestampMs = QDateTime::currentMSecsSinceEpoch();
    m_rules[r.domainKey] = r;
    save();
}

void ChoiceStore::removeRule(const QString& domainKey) {
    m_rules.remove(domainKey);
    save();
}

QList<ChoiceRule> ChoiceStore::rules() const { return m_rules.values(); }

QString ChoiceStore::baseDomainFromHost(const QString& host) {
    QString h = host.toLower();
    QStringList parts = h.split('.');
    if (parts.size() >= 2) {
        return parts[parts.size()-2] + "." + parts[parts.size()-1];
    }
    return h;
}

void ChoiceStore::load() {
    m_rules.clear();
    QSettings s(ORG, APP);
    int n = s.beginReadArray("choices");
    for (int i = 0; i < n; ++i) {
        s.setArrayIndex(i);
        ChoiceRule r;
        r.domainKey = s.value("domainKey").toString();
        r.configId = s.value("configId").toString();
        r.mode = static_cast<DomainMatchMode>(s.value("mode", static_cast<int>(DomainMatchMode::FullHost)).toInt());
        r.timestampMs = s.value("ts", 0).toLongLong();
        if (!r.domainKey.isEmpty() && !r.configId.isEmpty())
            m_rules[r.domainKey] = r;
    }
    s.endArray();
}

void ChoiceStore::save() const {
    QSettings s(ORG, APP);
    s.beginWriteArray("choices");
    int idx = 0;
    for (const auto& r : m_rules) {
        s.setArrayIndex(idx++);
        s.setValue("domainKey", r.domainKey);
        s.setValue("configId", r.configId);
        s.setValue("mode", static_cast<int>(r.mode));
        s.setValue("ts", r.timestampMs);
    }
    s.endArray();
}
