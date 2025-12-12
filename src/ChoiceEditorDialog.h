#pragma once
#include <QDialog>

class QListWidget;
class BrowserManager;
class ChoiceStore;

class ChoiceEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChoiceEditorDialog(BrowserManager* mgr, ChoiceStore* store, QWidget* parent = nullptr);

private slots:
    void onRemoveClicked();
    void onClearAllClicked();

private:
    BrowserManager* m_mgr;
    ChoiceStore* m_store;
    QListWidget* m_list;

    void populate();
};
