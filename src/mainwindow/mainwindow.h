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
#include "iconprovider.h"
#include "i18n.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

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

    QSplitter *mainSplitter;
    QTreeView *sidebar;
    QTreeView *fileView;
    QFileSystemModel *model;
    QLineEdit *searchBar;
    QTabWidget *tabs;
    QLabel *statusLabel;
    QLabel *statusSelectionLabel;
    QLabel *statusThemeLabel;
    WinIconProvider iconProvider;
    QStandardItemModel *sideModel;
    QStandardItem *trashItem;
    QWidget *breadcrumbWidget;
    QHBoxLayout *breadcrumbLayout;
    QStringList breadcrumbPaths;
    QWidget *topTabBarHost;
    QHBoxLayout *topTabBarLayout;
    QToolButton *topTabAddButton;

    // NEU: Settings-Button in Sidebar
    QPushButton *settingsBtn;

    QString trashPath;
    QString clipboardPath;
    bool isCut;
    bool showHidden;
    QStringList historyStack;
    int historyIndex;
    int searchGeneration;
    QTimer *searchTimer;
};