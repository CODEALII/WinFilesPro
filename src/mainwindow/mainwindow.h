#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QFileSystemModel>
#include <QToolBar>
#include <QLineEdit>
#include <QMenu>
#include <QAction>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupRibbon();
    void setupLayout();
    void setupSidebar();
    void setupStatusBar();
    void showContextMenu(const QPoint &pos);
    void navigateTo(const QString &path);
    void pasteHere();
    void deleteSelected();
    void renameSelected();
    void copySelected();
    void cutSelected();

    QAction *copyAction;
    QAction *cutAction;
    QAction *pasteAction;
    QAction *deleteAction;
    QAction *renameAction;
    QAction *newFolderAction;
    QAction *propertiesAction;
    QAction *backAction;
    QAction *forwardAction;
    QAction *upAction;

    QSplitter *mainSplitter;
    QTreeView *sidebar;
    QTreeView *fileView;
    QFileSystemModel *model;
    QFileSystemModel *sidebarModel;
    QLineEdit *addressBar;

    QString clipboardPath;
    bool isCut = false;
    QStringList historyStack;
    int historyIndex = -1;
};