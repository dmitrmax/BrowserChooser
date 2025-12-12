#pragma once
#include <QString>
#include <QMap>
#include <QSettings>

enum class DomainMatchMode {
    FullHost,
    BaseDomain
};

struct ChoiceRule {
    QString domainKey;
    DomainMatchMode mode;
    QString configId;
    qint64 timestampMs;
};

class ChoiceStore {
public:
    ChoiceStore();

    QString matchConfigIdForHost(const QString& host) const;
    void setRule(const ChoiceRule& rule);
    void removeRule(const QString& domainKey);
    QList<ChoiceRule> rules() const;

private:
    QMap<QString, ChoiceRule> m_rules;
    static QString baseDomainFromHost(const QString& host);
    void load();
    void save() const;
};
