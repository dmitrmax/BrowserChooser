#pragma once
#include <QMainWindow>
#include "BrowserManager.h"
#include "ChoiceStore.h"

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLabel;
class QLineEdit;
class QCheckBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(BrowserManager* mgr, ChoiceStore* store,
                        const QString& urlParam = QString(), QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onOpenClicked();
    void onEditChoicesClicked();
    void onNewConfigClicked();
    void onEditConfigClicked();
    void onDeleteConfigClicked();
    void onCloneConfigClicked();
    void onCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

private:
    void keyPressEvent(QKeyEvent *event) override;

private:
    BrowserManager* m_mgr;
    ChoiceStore* m_store;
    QString m_url;

    QListWidget* m_list;
    QLabel* m_urlLabel;
    QLineEdit* m_urlEdit;
    QCheckBox* m_rememberCheck;
    QCheckBox* m_baseDomainCheck;
    QCheckBox* m_checkDefaultOnStartup;
    QCheckBox* m_ignoreDefaultBrowserEnv;
    QCheckBox* m_unwrapSlackUrls;
    QPushButton* m_editCfgBtn;
    QPushButton* m_delCfgBtn;

    void populateList();
    QString currentDomainKey(DomainMatchMode* outMode) const;
    void autoSelectFromRemembered();

    void launchConfig(const BrowserConfig& cfg, const QString& url);

    void checkAndOfferDefault();

    // Helpers: default handler check on Linux
    QString defaultHandlerForScheme(const QString& scheme) const;
    QString desktopFileId() const;
};
