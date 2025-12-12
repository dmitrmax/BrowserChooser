#include "ChoiceEditorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>

#include "BrowserManager.h"
#include "ChoiceStore.h"

ChoiceEditorDialog::ChoiceEditorDialog(BrowserManager* mgr, ChoiceStore* store, QWidget* parent)
    : QDialog(parent), m_mgr(mgr), m_store(store)
{
    auto* v = new QVBoxLayout(this);
    m_list = new QListWidget(this);
    v->addWidget(m_list);

    auto* btnRow = new QHBoxLayout();
    auto* removeBtn = new QPushButton(tr("Remove selected"), this);
    auto* clearBtn = new QPushButton(tr("Clear all"), this);
    btnRow->addWidget(removeBtn);
    btnRow->addWidget(clearBtn);
    btnRow->addStretch();
    v->addLayout(btnRow);

    connect(removeBtn, &QPushButton::clicked, this, &ChoiceEditorDialog::onRemoveClicked);
    connect(clearBtn, &QPushButton::clicked, this, &ChoiceEditorDialog::onClearAllClicked);

    setWindowTitle(tr("Remembered domain choices"));
    resize(480, 360);
    populate();
}

void ChoiceEditorDialog::populate() {
    m_list->clear();
    for (const auto& r : m_store->rules()) {
        static const QString UNKNOWN(tr("<unknown>"));

        BrowserConfig* cfg = m_mgr->findById(r.configId);
        auto& name = cfg ? cfg->name : UNKNOWN;

        QString mode = (r.mode == DomainMatchMode::BaseDomain) ? tr("base") : tr("host");
        auto* item = new QListWidgetItem(QString("%1 (%2) -> %3")
            .arg(r.domainKey).arg(mode).arg(name), m_list);
        item->setData(Qt::UserRole, r.domainKey);
    }
}

void ChoiceEditorDialog::onRemoveClicked() {
    auto* item = m_list->currentItem();
    if (!item) return;
    QString key = item->data(Qt::UserRole).toString();
    m_store->removeRule(key);
    populate();
}

void ChoiceEditorDialog::onClearAllClicked() {
    auto ret = QMessageBox::question(this, tr("Clear all"), tr("Remove all remembered choices?"));
    if (ret != QMessageBox::Yes) return;
    for (const auto& r : m_store->rules()) {
        m_store->removeRule(r.domainKey);
    }
    populate();
}
