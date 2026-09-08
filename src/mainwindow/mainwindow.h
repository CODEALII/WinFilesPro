#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QToolBar>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QTabWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QScrollArea>
#include <QTimer>
#include <QProcess>
#include <QShowEvent>
#include "iconprovider.h"
#include "i18n.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openPath(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupRibbon();
    void setupLayout();
    void setupStatusBar();
    void applyTheme();
    void showContextMenu(const QPoint &pos);
    void navigateTo(const QString &path, bool addHistory = true);
    void navigateTabTo(const QString &path);
    void openNewTab(const QString &path);
    void closeCurrentTab();
    void pasteHere();
    void deleteSelected();
    void renameSelected();
    void copySelected();
    void cutSelected();
    void refreshCurrentView();
    void updateSelectionActions();
    void updateAddressBar();
    void updateBreadcrumbs(const QString &path);
    void startPathEdit();
    void commitPathEdit(const QString &text);
    void openFileWithAssoc(const QString &path);
    void launchApp(const QString &app, const QString &file);
    void convertToPng(const QString &path);
    void updateStatusDetails();
    void showAboutDialog();
    void showHelpDialog();
    void showSortMenu();
    void showViewMenu();
    void toggleTheme();
    void createNewFile();
    void copyPathToClipboard();
    void openInTerminal();
    void toggleHiddenFiles();
    bool rowHiddenByPolicy(const QModelIndex &index) const;
    void applyHiddenPolicyToView(QTreeView *view);
    void applyHiddenPolicyToAllViews();
    QModelIndexList visibleSelectedRows() const;
    void onSearchTextChanged(const QString &text);
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);
    void onSidebarClicked(const QModelIndex &index);
    void onFileDoubleClicked(const QModelIndex &index);
    void onFileSelectionChanged();
    void onNewFolderClicked();
    void onPropertiesClicked();
    void onBackClicked();
    void onForwardClicked();
    void onUpClicked();
    void onRefreshClicked();
    void setupTopTabBar();
    void refreshTopTabBar();

    // NEU: Settings
    void openSettings();

    QAction *copyAction;
    QAction *cutAction;
    QAction *pasteAction;
    QAction *deleteAction;
    QAction *renameAction;
    QAction *newFolderAction;
    QAction *newFileAction;
    QAction *propertiesAction;
    QAction *refreshAction;
    QAction *aboutAction;
    QAction *helpAction;
    QAction *newTabAction;
    QAction *closeTabAction;
    QAction *backAction;
    QAction *forwardAction;
    QAction *upAction;
    QAction *sortAction;
    QAction *viewAction;
    QAction *themeAction;
    QAction *hiddenFilesAction;
    QAction *terminalAction;

    QSplitter *mainSplitter = nullptr;
    QTreeView *sidebar = nullptr;
    QTreeView *fileView;
    QFileSystemModel *model;
    QLineEdit *searchBar;
    QLineEdit *pathEditor = nullptr;
    QTabWidget *tabs;
    QLabel *statusLabel;
    QLabel *statusSelectionLabel;
    QLabel *statusThemeLabel;
    WinIconProvider iconProvider;
    QStandardItemModel *sideModel;
    QStandardItem *trashItem;
    QToolBar *navToolbar = nullptr;
    QToolBar *cmdToolbar = nullptr;
    QWidget *breadcrumbWidget;
    QHBoxLayout *breadcrumbLayout;
    QStringList breadcrumbPaths;
    QWidget *topTabBarHost;
    QHBoxLayout *topTabBarLayout;
    QToolBar *tabBarToolbar = nullptr;
    QToolButton *topTabAddButton;

    // NEU: Settings-Button in Sidebar
    QPushButton *settingsBtn;

    void maybeShowAdminToast(const QString &path);
    void updateAdminToastPosition();
    void addPinnedItem(const QString &path);
    void showSidebarContextMenu(const QPoint &pos);
    void launchElevated(const QString &path);
    void stopElevationWait();
    void duplicateCurrentTab();
    void openTabContextMenu(const QPoint &pos);
    void createZip();
    void extractZip();

    QWidget *adminToast = nullptr;
    QString adminToastPath;
    QStringList pinnedPaths;

    QProcess *elevateProcess = nullptr;
    QTimer *elevatePoll = nullptr;
    QString elevateMarker;
    QString elevatedStartPath;
    bool windowShownOnce = false;

    QString trashPath;
    QString clipboardPath;
    bool isCut;
    bool showHidden;
    QStringList historyStack;
    int historyIndex;
    int searchGeneration;
    QTimer *searchTimer;
};