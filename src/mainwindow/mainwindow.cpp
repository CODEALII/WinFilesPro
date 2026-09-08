#include "mainwindow.h"
#include "style.h"
#include "settingsmanager.h"
#include <QApplication>
#include <QCloseEvent>
#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QDesktopServices>
#include <QStorageInfo>
#include <QStatusBar>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QToolButton>
#include <QShortcut>
#include <QKeySequence>
#include <QPainter>
#include <QMouseEvent>
#include <QClipboard>
#include <QProcess>

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>

#include "settingsdialog.h"
#include "updater.h"

#include <QIcon>
#include <QPixmap>
#include <QColor>

#include <QScrollArea>
#include <QProgressBar>
#include <QFrame>
#include <functional>

// ---------------------------------------------------------------------------
// Hilfsfunktion: SVG-Icon in einer bestimmten Farbe einfärben
// ---------------------------------------------------------------------------
static QIcon colorizeIcon(const QString &path, const QColor &color) {
    QPixmap pixmap = QIcon(path).pixmap(QSize(24, 24));
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    painter.end();
    return QIcon(pixmap);
}

// ---------------------------------------------------------------------------
// Hilfsfunktion: Dateigröße menschenlesbar formatieren
// ---------------------------------------------------------------------------
static QString formatSize(qint64 bytes) {
    const double kb = 1024.0, mb = kb * 1024.0, gb = mb * 1024.0;
    if (bytes >= gb) return QString::number(bytes / gb, 'f', 2) + " GB";
    if (bytes >= mb) return QString::number(bytes / mb, 'f', 2) + " MB";
    if (bytes >= kb) return QString::number(bytes / kb, 'f', 1) + " KB";
    return QString::number(bytes) + " Bytes";
}

// ---------------------------------------------------------------------------
// DriveCard - Kachel für "Dieser PC" im Win11-Look
// ---------------------------------------------------------------------------
class DriveCard : public QFrame {
public:
    DriveCard(const QString &name, const QString &path, qint64 freeBytes, qint64 totalBytes,
               std::function<void(QString)> clickCallback, QWidget *parent = nullptr)
        : QFrame(parent), drivePath(path), onClick(clickCallback) {

        const ThemeColors &c = AppStyle::colors();

        setFixedSize(290, 82);
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QString(R"(
            DriveCard {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 8px;
            }
            DriveCard:hover {
                background-color: %3;
                border: 1px solid %4;
            }
        )").arg(c.surfaceBg, c.border, c.hoverBg, c.accent));

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(14, 10, 14, 10);
        layout->setSpacing(14);

        QLabel *iconLabel = new QLabel();
        iconLabel->setPixmap(QIcon(":/icons/hardware.ico").pixmap(38, 38));
        layout->addWidget(iconLabel);

        QVBoxLayout *vbox = new QVBoxLayout();
        vbox->setAlignment(Qt::AlignVCenter);
        vbox->setSpacing(4);

        QLabel *nameLabel = new QLabel(name);
        nameLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600; border: none; background: transparent;").arg(c.textPrimary));
        vbox->addWidget(nameLabel);

        QProgressBar *prog = new QProgressBar();
        prog->setFixedHeight(6);
        prog->setTextVisible(false);
        prog->setRange(0, 100);
        double used = totalBytes - freeBytes;
        int pct = totalBytes > 0 ? int((used / (double)totalBytes) * 100) : 0;
        prog->setValue(pct);

        QString barColor = (pct > 90) ? c.danger : (pct > 75 ? c.warning : c.accent);
        prog->setStyleSheet(QString(R"(
            QProgressBar {
                border: none;
                border-radius: 3px;
                background-color: %1;
            }
            QProgressBar::chunk {
                background-color: %2;
                border-radius: 3px;
            }
        )").arg(c.borderSubtle, barColor));

        vbox->addWidget(prog);

        double freeGb = freeBytes / 1e9;
        double totalGb = totalBytes / 1e9;
        QLabel *descLabel = new QLabel(QString("%1 GB frei von %2 GB").arg(freeGb, 0, 'f', 1).arg(totalGb, 0, 'f', 1));
        descLabel->setStyleSheet(QString("color: %1; font-size: 11px; border: none; background: transparent;").arg(c.textSecondary));
        vbox->addWidget(descLabel);

        layout->addLayout(vbox);
        layout->setStretch(1, 1);
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && onClick) {
            onClick(drivePath);
        }
    }

private:
    QString drivePath;
    std::function<void(QString)> onClick;
};

// ---------------------------------------------------------------------------
// ClickableLabel - einfaches QLabel mit clicked()-Signal (für die Statusleiste)
// ---------------------------------------------------------------------------
class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(const QString &text, QWidget *parent = nullptr) : QLabel(text, parent) {}
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) emit clicked();
        QLabel::mousePressEvent(event);
    }
};

// ---------------------------------------------------------------------------
// ModernConfirmDialog - rahmenloser Bestätigungsdialog mit Schatten
// ---------------------------------------------------------------------------
class ModernConfirmDialog : public QDialog {
public:
    ModernConfirmDialog(const QString &title, const QString &message, QWidget *parent = nullptr,
                          const QString &confirmText = T("delete"), bool danger = true)
        : QDialog(parent) {

        const ThemeColors &c = AppStyle::colors();

        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(380, 240);

        QWidget *container = new QWidget(this);
        container->setObjectName("confirmContainer");
        container->setStyleSheet(QString(R"(
            QWidget#confirmContainer {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 10px;
            }
            QLabel#titleLabel {
                color: %3;
                font-size: 15px;
                font-weight: 700;
                border: none;
                background: transparent;
            }
            QLabel#messageLabel {
                color: %4;
                font-size: 13px;
                border: none;
                background: transparent;
            }
        )").arg(c.elevatedBg, c.border, c.textPrimary, c.textSecondary));
            
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(18, 18, 18, 18);
        mainLayout->addWidget(container);

        QVBoxLayout *innerLayout = new QVBoxLayout(container);
        innerLayout->setContentsMargins(22, 20, 22, 18);
        innerLayout->setSpacing(10);

        QLabel *titleLabel = new QLabel(title);
        titleLabel->setObjectName("titleLabel");
        innerLayout->addWidget(titleLabel);

        QLabel *messageLabel = new QLabel(message);
        messageLabel->setObjectName("messageLabel");
        messageLabel->setWordWrap(true);
        innerLayout->addWidget(messageLabel);

        innerLayout->addStretch();

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *cancelBtn = new QPushButton(T("cancel"));
        QPushButton *confirmBtn = new QPushButton(confirmText);
        confirmBtn->setObjectName(danger ? "dangerBtn" : "primaryBtn");

        buttonLayout->addStretch();
        buttonLayout->addWidget(cancelBtn);
        buttonLayout->addWidget(confirmBtn);
        innerLayout->addLayout(buttonLayout);

        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        connect(confirmBtn, &QPushButton::clicked, this, &QDialog::accept);

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(28);
        shadow->setColor(QColor(0, 0, 0, 90));
        shadow->setOffset(0, 6);
        container->setGraphicsEffect(shadow);
    }
};

// ---------------------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      fileView(nullptr),
      model(nullptr),
      searchBar(nullptr),
      tabs(nullptr),
      statusLabel(nullptr),
      statusSelectionLabel(nullptr),
      statusThemeLabel(nullptr),
      sideModel(nullptr),
      trashItem(nullptr),
      breadcrumbWidget(nullptr),
      breadcrumbLayout(nullptr),
      isCut(false),
      showHidden(false),
      historyIndex(-1),
      searchGeneration(0),
      searchTimer(nullptr) {

#ifdef Q_OS_WIN32
    trashPath = QString();
#elif defined(Q_OS_MAC)
    trashPath = QDir::homePath() + "/.Trash";
#else
    trashPath = QDir::homePath() + "/.local/share/Trash/files";
#endif

    setWindowIcon(QIcon(":/icons/explorer.ico"));
    setMinimumSize(960, 620);

    applyTheme();

    setupRibbon();
    setupStatusBar();
    setupLayout();
    navigateTo(QDir::homePath());
}

void MainWindow::applyTheme() {
    // set qapp stylesheet to dark!
    qApp->setStyleSheet(AppStyle::globalStyleSheet());
}

void MainWindow::setupRibbon() {
    const ThemeColors &c = AppStyle::colors();
    QColor iconColor(c.textPrimary);

    QToolBar *navToolbar = addToolBar("Navigation");
    navToolbar->setMovable(false);
    navToolbar->setFloatable(false);
    navToolbar->setIconSize(QSize(16, 16));

    backAction    = navToolbar->addAction(colorizeIcon(":/icons/arw_left.svg", iconColor), T("back"));
    forwardAction = navToolbar->addAction(colorizeIcon(":/icons/arw_right.svg", iconColor), T("forward"));
    upAction      = navToolbar->addAction(colorizeIcon(":/icons/arw_up.svg", iconColor), T("up"));
    refreshAction = navToolbar->addAction(colorizeIcon(":/icons/refresh.svg", iconColor), T("refresh"));

    backAction->setToolTip(T("back_tt"));
    forwardAction->setToolTip(T("forward_tt"));
    upAction->setToolTip(T("up_tt"));
    refreshAction->setToolTip(T("refresh_tt"));

    navToolbar->addSeparator();

    breadcrumbWidget = new QWidget;
    breadcrumbWidget->setObjectName("breadcrumbWidget");
    breadcrumbLayout = new QHBoxLayout(breadcrumbWidget);
    breadcrumbLayout->setContentsMargins(6, 2, 6, 2);
    breadcrumbLayout->setSpacing(2);
    navToolbar->addWidget(breadcrumbWidget);

    searchBar = new QLineEdit;
    searchBar->setPlaceholderText(T("search"));
    searchBar->setFixedWidth(220);
    searchBar->setClearButtonEnabled(true);
    QAction *searchIcon = new QAction(colorizeIcon(":/icons/search.svg", QColor(c.textSecondary)), "", this);
    searchBar->addAction(searchIcon, QLineEdit::LeadingPosition);
    navToolbar->addWidget(searchBar);

    navToolbar->addSeparator();

    newTabAction = navToolbar->addAction(colorizeIcon(":/icons/new_file.svg", iconColor), T("new_tab"));
    newTabAction->setToolTip(T("new_tab_tt"));
    closeTabAction = navToolbar->addAction(colorizeIcon(":/icons/delete.svg", iconColor), T("close_tab"));
    closeTabAction->setToolTip(T("close_tab_tt"));

    QToolBar *cmdToolbar = addToolBar("Befehle");
    cmdToolbar->setMovable(false);
    cmdToolbar->setFloatable(false);
    cmdToolbar->setIconSize(QSize(16, 16));

    newFolderAction = cmdToolbar->addAction(colorizeIcon(":/icons/new_folder.svg", iconColor), T("new_folder"));
    newFileAction   = cmdToolbar->addAction(colorizeIcon(":/icons/new_file.svg", iconColor), T("new_file"));
    cmdToolbar->addSeparator();
    cutAction    = cmdToolbar->addAction(colorizeIcon(":/icons/cut.svg", iconColor), T("cut"));
    copyAction   = cmdToolbar->addAction(colorizeIcon(":/icons/copy.svg", iconColor), T("copy"));
    pasteAction  = cmdToolbar->addAction(colorizeIcon(":/icons/paste.svg", iconColor), T("paste"));
    renameAction = cmdToolbar->addAction(colorizeIcon(":/icons/rename.svg", iconColor), T("rename"));
    deleteAction = cmdToolbar->addAction(colorizeIcon(":/icons/delete.svg", iconColor), T("delete"));
    cmdToolbar->addSeparator();
    sortAction = cmdToolbar->addAction(colorizeIcon(":/icons/sort.svg", iconColor), T("sort"));
    viewAction = cmdToolbar->addAction(colorizeIcon(":/icons/view.svg", iconColor), T("view"));
    hiddenFilesAction = cmdToolbar->addAction(colorizeIcon(":/icons/hidden.svg", iconColor), T("hidden_files"));
    hiddenFilesAction->setCheckable(true);
    cmdToolbar->addSeparator();
    terminalAction   = cmdToolbar->addAction(colorizeIcon(":/icons/terminal.svg", iconColor), T("terminal"));
    propertiesAction = cmdToolbar->addAction(colorizeIcon(":/icons/info.svg", iconColor), T("properties"));

    newFolderAction->setToolTip(T("new_folder_tt"));
    newFileAction->setToolTip(T("new_file_tt"));
    cutAction->setToolTip(T("cut_tt"));
    copyAction->setToolTip(T("copy_tt"));
    pasteAction->setToolTip(T("paste_tt"));
    renameAction->setToolTip(T("rename_tt"));
    deleteAction->setToolTip(T("delete_tt"));
    hiddenFilesAction->setToolTip(T("hidden_tt"));
    terminalAction->setToolTip(T("terminal_tt"));
    propertiesAction->setToolTip(T("properties_tt"));

    copyAction->setEnabled(false);
    cutAction->setEnabled(false);
    pasteAction->setEnabled(false);
    deleteAction->setEnabled(false);
    renameAction->setEnabled(false);
    propertiesAction->setEnabled(false);
    backAction->setEnabled(false);
    forwardAction->setEnabled(false);

    connect(backAction, &QAction::triggered, this, &MainWindow::onBackClicked);
    connect(forwardAction, &QAction::triggered, this, &MainWindow::onForwardClicked);
    connect(upAction, &QAction::triggered, this, &MainWindow::onUpClicked);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshClicked);
    connect(searchBar, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(newFolderAction, &QAction::triggered, this, &MainWindow::onNewFolderClicked);
    connect(newFileAction, &QAction::triggered, this, &MainWindow::createNewFile);
    connect(cutAction, &QAction::triggered, this, &MainWindow::cutSelected);
    connect(copyAction, &QAction::triggered, this, &MainWindow::copySelected);
    connect(pasteAction, &QAction::triggered, this, &MainWindow::pasteHere);
    connect(renameAction, &QAction::triggered, this, &MainWindow::renameSelected);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelected);
    connect(propertiesAction, &QAction::triggered, this, &MainWindow::onPropertiesClicked);
    connect(sortAction, &QAction::triggered, this, &MainWindow::showSortMenu);
    connect(viewAction, &QAction::triggered, this, &MainWindow::showViewMenu);
    connect(hiddenFilesAction, &QAction::triggered, this, &MainWindow::toggleHiddenFiles);
    connect(terminalAction, &QAction::triggered, this, &MainWindow::openInTerminal);
    connect(newTabAction, &QAction::triggered, this, [this] { openNewTab(QDir::homePath()); });
    connect(closeTabAction, &QAction::triggered, this, &MainWindow::closeCurrentTab);

    // Tastenkürzel
    new QShortcut(QKeySequence("F5"), this, this, &MainWindow::onRefreshClicked);
    new QShortcut(QKeySequence("F2"), this, this, &MainWindow::renameSelected);
    new QShortcut(QKeySequence::Delete, this, this, &MainWindow::deleteSelected);
    new QShortcut(QKeySequence::Copy, this, this, &MainWindow::copySelected);
    new QShortcut(QKeySequence::Cut, this, this, &MainWindow::cutSelected);
    new QShortcut(QKeySequence::Paste, this, this, &MainWindow::pasteHere);
    new QShortcut(QKeySequence("Ctrl+Shift+N"), this, this, &MainWindow::onNewFolderClicked);
    new QShortcut(QKeySequence("Ctrl+H"), this, this, &MainWindow::toggleHiddenFiles);
    new QShortcut(QKeySequence("Ctrl+T"), this, this, [this] { openNewTab(QDir::homePath()); });
    new QShortcut(QKeySequence("Ctrl+W"), this, this, &MainWindow::closeCurrentTab);
    new QShortcut(QKeySequence("Alt+Left"), this, this, &MainWindow::onBackClicked);
    new QShortcut(QKeySequence("Alt+Right"), this, this, &MainWindow::onForwardClicked);
    new QShortcut(QKeySequence("Alt+Up"), this, this, &MainWindow::onUpClicked);
}

void MainWindow::setupLayout() {
    model = new QFileSystemModel(this);
    model->setIconProvider(&iconProvider);
    model->setRootPath("/");
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    sideModel = new QStandardItemModel(this);

    QStandardItem *quick = new QStandardItem(QIcon(":/icons/folder_favourites.ico"), "Schnellzugriff");
    quick->setData("__header__", Qt::UserRole);
    quick->setEditable(false);
    sideModel->appendRow(quick);

    struct QuickItem { QString name; QString path; QString icon; };
    QList<QuickItem> quickItems = {
        {"Desktop", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), ":/icons/folder_desktop.ico"},
        {"Downloads", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), ":/icons/folder_downloads.ico"},
        {"Dokumente", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), ":/icons/folder_documents.ico"},
        {"Bilder", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), ":/icons/folder_pictures.ico"},
        {"Musik", QStandardPaths::writableLocation(QStandardPaths::MusicLocation), ":/icons/folder_music.ico"},
        {"Videos", QStandardPaths::writableLocation(QStandardPaths::MoviesLocation), ":/icons/folder_videos.ico"}
    };
    for (const auto &itemData : quickItems) {
        QStandardItem *item = new QStandardItem(QIcon(itemData.icon), itemData.name);
        item->setData(itemData.path, Qt::UserRole);
        item->setEditable(false);
        quick->appendRow(item);
    }

    bool trashFull = false;
    if (!trashPath.isEmpty()) {
        trashFull = QDir(trashPath).exists() && !QDir(trashPath).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
    }
    trashItem = new QStandardItem(QIcon(trashFull ? ":/icons/trash_full.ico" : ":/icons/trash_empty.ico"), "Papierkorb");
    trashItem->setData(!trashPath.isEmpty() && QDir(trashPath).exists() ? trashPath : QString(), Qt::UserRole);
    trashItem->setEditable(false);
    sideModel->appendRow(trashItem);

    QStandardItem *computer = new QStandardItem(QIcon(":/icons/computer.ico"), "Dieser PC");
    computer->setData("computer://", Qt::UserRole);
    computer->setEditable(false);
    sideModel->appendRow(computer);

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (!storage.isValid() || !storage.isReady()) continue;
        QString path = storage.rootPath();
        if (path != "/" && !path.startsWith("/media") && !path.startsWith("/mnt") && !path.startsWith("/run/media")) continue;
        double gb = storage.bytesTotal() / 1e9;
        QString name = storage.displayName().isEmpty() ? path : storage.displayName();
        if (storage.isRoot()) name += " (System)";
        QStandardItem *drive = new QStandardItem(QIcon(":/icons/hardware.ico"), QString("%1 (%2 GB)").arg(name).arg(gb, 0, 'f', 0));
        drive->setData(path, Qt::UserRole);
        drive->setEditable(false);
        computer->appendRow(drive);
    }

    sidebar = new QTreeView;
    sidebar->setObjectName("sidebar");
    sidebar->setModel(sideModel);
    sidebar->setFocusPolicy(Qt::NoFocus);
    sidebar->setHeaderHidden(true);
    sidebar->setIconSize(QSize(20, 20));
    sidebar->setIndentation(16);
    sidebar->setMinimumWidth(220);
    sidebar->setMaximumWidth(300);
    sidebar->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sidebar->setContextMenuPolicy(Qt::NoContextMenu);
    sidebar->expandAll();
    connect(sidebar, &QTreeView::clicked, this, &MainWindow::onSidebarClicked);

    // QTabWidget bleibt vollkommen normal - NICHTS wird aus ihm herausgerissen.
    tabs = new QTabWidget;
    tabs->setObjectName("mainTabs");
    tabs->setTabsClosable(true);
    tabs->setMovable(true);
    tabs->setDocumentMode(true);
    tabs->tabBar()->hide(); // die eingebaute Tableiste wird nur AUSGEBLENDET, nicht entfernt
    connect(tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(tabs, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);

    openNewTab(QDir::homePath());

    mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(sidebar);
    mainSplitter->addWidget(tabs);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setSizes({240, 860});

    // Settings-Button unten in der Sidebar
    const ThemeColors &c = AppStyle::colors();
    settingsBtn = new QPushButton("⚙  " + T("settings"));
    settingsBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            border-top: 1px solid %1;
            border-radius: 0;
            color: %2;
            font-size: 13px;
            padding: 12px 16px;
            text-align: left;
        }
        QPushButton:hover { background-color: %3; }
        QPushButton:pressed { background-color: %4; }
    )").arg(c.border, c.textSecondary, c.hoverBg, c.pressedBg));
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::openSettings);

    // Sidebar in ein Widget wrappen damit Settings-Button ganz unten ist
    QWidget *sidebarHost = new QWidget;
    sidebarHost->setStyleSheet(QString("background-color: %1;").arg(c.sidebarBg));
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebarHost);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);
    sidebarLayout->addWidget(sidebar);
    sidebarLayout->addWidget(settingsBtn);

    mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(sidebarHost);
    mainSplitter->addWidget(tabs);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setSizes({240, 860});

    setCentralWidget(mainSplitter);

    setupTopTabBar();
}

// Eigene, rein optische Tab-Leiste ganz oben im Fenster, die den echten
// (versteckten) tabs->tabBar() spiegelt. Bei Klick wird nur
// tabs->setCurrentIndex(i) aufgerufen - die eigentliche Tab-Logik bleibt
// unverändert in QTabWidget.
void MainWindow::setupTopTabBar() {
    topTabBarHost = new QWidget;
    topTabBarHost->setObjectName("topTabBarHost");

    QHBoxLayout *layout = new QHBoxLayout(topTabBarHost);
    layout->setContentsMargins(8, 4, 8, 0);
    layout->setSpacing(4);
    topTabBarLayout = layout;

    QToolButton *addBtn = new QToolButton;
    addBtn->setObjectName("addTabBtn");
    addBtn->setText("+");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setToolTip("Neuer Tab (Strg+T)");
    connect(addBtn, &QToolButton::clicked, this, [this] { openNewTab(QDir::homePath()); });

    QToolBar *tabBarHost = new QToolBar("Tabs");
    tabBarHost->setObjectName("tabBarToolbar");
    tabBarHost->setMovable(false);
    tabBarHost->setFloatable(false);
    tabBarHost->addWidget(topTabBarHost);
    addToolBar(Qt::TopToolBarArea, tabBarHost);
    insertToolBarBreak(tabBarHost);

    topTabAddButton = addBtn;
    refreshTopTabBar();
}

// Baut die sichtbaren Tab-Buttons oben neu auf - wird nach jeder
// Tab-Änderung (öffnen/schließen/umbenennen/aktiv werden) aufgerufen.
void MainWindow::refreshTopTabBar() {
    if (!topTabBarLayout) return;

    QLayoutItem *item;
    while ((item = topTabBarLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    for (int i = 0; i < tabs->count(); ++i) {
        QToolButton *tabBtn = new QToolButton;
        tabBtn->setObjectName("topTabButton");
        tabBtn->setCheckable(true);
        tabBtn->setChecked(i == tabs->currentIndex());
        tabBtn->setText(tabs->tabText(i));
        tabBtn->setIcon(tabs->tabIcon(i));
        tabBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        tabBtn->setCursor(Qt::PointingHandCursor);
        int index = i;
        connect(tabBtn, &QToolButton::clicked, this, [this, index] { tabs->setCurrentIndex(index); });
        topTabBarLayout->addWidget(tabBtn);

        if (tabs->count() > 1) {
            QToolButton *closeBtn = new QToolButton;
            closeBtn->setObjectName("topTabCloseButton");
            closeBtn->setText("×");
            closeBtn->setCursor(Qt::PointingHandCursor);
            connect(closeBtn, &QToolButton::clicked, this, [this, index] { onTabCloseRequested(index); });
            topTabBarLayout->addWidget(closeBtn);
        }
    }

    topTabBarLayout->addWidget(topTabAddButton);
    topTabBarLayout->addStretch();
}

void MainWindow::setupStatusBar() {
    statusLabel = new QLabel(T("ready"));
    statusBar()->addWidget(statusLabel, 1);

    statusSelectionLabel = new QLabel("");
    statusBar()->addPermanentWidget(statusSelectionLabel);

    QFrame *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    statusBar()->addPermanentWidget(sep);
}

void MainWindow::onSidebarClicked(const QModelIndex &index) {
    QString path = sideModel->itemFromIndex(index)->data(Qt::UserRole).toString();

    if (path == "computer://") {
        openNewTab("computer://");
        return;
    }

    if (!path.isEmpty() && path != "__header__") {
        navigateTo(path);
    }
}

void MainWindow::onFileDoubleClicked(const QModelIndex &index) {
    if (!fileView) return;
    if (model->isDir(index)) navigateTo(model->filePath(index));
    else QDesktopServices::openUrl(QUrl::fromLocalFile(model->filePath(index)));
}

void MainWindow::onFileSelectionChanged() {
    updateSelectionActions();
    updateStatusDetails();
}

void MainWindow::onNewFolderClicked() {
    if (!fileView) return;
    bool ok = false;
    QString name = QInputDialog::getText(this, T("folder_dialog"), T("folder_label"), QLineEdit::Normal, T("new_folder"), &ok);
    if (!ok || name.isEmpty()) return;

    if (name.contains('/') || name.contains('\\') || name.contains("..")) {
        ModernConfirmDialog dlg(T("error"), T("invalid_chars"), this, T("ok"), false);
        dlg.exec();
        return;
    }

    if (!QDir(model->filePath(fileView->rootIndex())).mkdir(name)) {
        ModernConfirmDialog dlg(T("error"), T("folder_create_fail"), this, T("ok"), false);
        dlg.exec();
        return;
    }
    refreshCurrentView();
}

void MainWindow::createNewFile() {
    if (!fileView) return;
    bool ok = false;
    QString name = QInputDialog::getText(this, T("file_dialog"), T("file_label"), QLineEdit::Normal, T("new_file") + ".txt", &ok);
    if (!ok || name.isEmpty()) return;

    if (name.contains('/') || name.contains('\\') || name.contains("..")) {
        ModernConfirmDialog dlg(T("error"), T("invalid_chars"), this, T("ok"), false);
        dlg.exec();
        return;
    }

    QString fullPath = QDir(model->filePath(fileView->rootIndex())).filePath(name);
    QFile f(fullPath);
    if (f.exists()) {
        ModernConfirmDialog dlg(T("error"), T("file_exists"), this, T("ok"), false);
        dlg.exec();
        return;
    }
    if (f.open(QIODevice::WriteOnly)) {
        f.close();
        refreshCurrentView();
    } else {
        ModernConfirmDialog dlg(T("error"), T("file_fail"), this, T("ok"), false);
        dlg.exec();
    }
}

void MainWindow::onPropertiesClicked() {
    if (!fileView) return;
    QModelIndex index = fileView->currentIndex();
    if (!index.isValid()) return;
    QFileInfo info(model->filePath(index));
    QString type = info.isDir() ? T("type_folder")
                              : (info.suffix().isEmpty() ? T("type_file")
                                                         : info.suffix().toUpper() + "-" + T("type_file"));
    QString sizeStr = info.isDir() ? "-" : QString("%1").arg(formatSize(info.size()));
    ModernConfirmDialog dlg(T("properties"),
        QString("%1 %2\n%3 %4\n%5 %6\n%7 %8\n%9 %10")
        .arg(T("props_name"), info.fileName())
        .arg(T("props_path"), info.absoluteFilePath())
        .arg(T("props_type"), type)
        .arg(T("props_size"), sizeStr)
        .arg(T("props_modified"), info.lastModified().toString("dd.MM.yyyy hh:mm")), this, T("ok"), false);
    dlg.exec();

}

void MainWindow::onBackClicked() {
    if (historyIndex > 0) {
        --historyIndex;
        navigateTabTo(historyStack.at(historyIndex));
        updateSelectionActions();
        backAction->setEnabled(historyIndex > 0);
        forwardAction->setEnabled(historyIndex + 1 < historyStack.size());
    }
}

void MainWindow::onForwardClicked() {
    if (historyIndex + 1 < historyStack.size()) {
        ++historyIndex;
        navigateTabTo(historyStack.at(historyIndex));
        updateSelectionActions();
        backAction->setEnabled(historyIndex > 0);
        forwardAction->setEnabled(historyIndex + 1 < historyStack.size());
    }
}

void MainWindow::onUpClicked() {
    if (!fileView) return;
    QDir dir(model->filePath(fileView->rootIndex()));
    if (dir.cdUp()) navigateTo(dir.absolutePath());
}

void MainWindow::onRefreshClicked() {
    refreshCurrentView();
}

void MainWindow::toggleHiddenFiles() {
    showHidden = !showHidden;
    hiddenFilesAction->setChecked(showHidden);
    QDir::Filters f = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (showHidden) f |= QDir::Hidden;
    model->setFilter(f);
    refreshCurrentView();
}

void MainWindow::copyPathToClipboard() {
    if (!fileView) return;
    QString path = model->filePath(fileView->rootIndex());
    QModelIndex idx = fileView->currentIndex();
    if (idx.isValid()) path = model->filePath(idx);
    QApplication::clipboard()->setText(path);
    if (statusLabel) statusLabel->setText(T("path_copied") + path);
}

void MainWindow::openInTerminal() {
    if (!fileView) return;
    QString path = model->filePath(fileView->rootIndex());
    // Versucht gängige Terminals der Reihe nach, ohne von einem
    // bestimmten Desktop-/Distributions-Theme abhängig zu sein.
    const QStringList candidates = {"x-terminal-emulator", "gnome-terminal", "konsole", "xterm"};
    for (const QString &term : candidates) {
        if (QProcess::startDetached(term, {}, path)) return;
    }
    // use ModernConfirmDialog for a more modern look
    ModernConfirmDialog dlg(T("terminal"), T("no_terminal"), this, T("ok"), false);
    dlg.exec();
}

void MainWindow::onSearchTextChanged(const QString &text) {
    if (!searchTimer) {
        searchTimer = new QTimer(this);
        searchTimer->setSingleShot(true);
    }
    searchTimer->disconnect();
    connect(searchTimer, &QTimer::timeout, this, [this, text] {
        if (!fileView) return;
        if (text.trimmed().isEmpty()) {
            QModelIndex root = fileView->rootIndex();
            for (int row = 0; row < model->rowCount(root); ++row) {
                bool shouldHide = showHidden ? false : model->fileName(model->index(row, 0, root)).startsWith('.');
                fileView->setRowHidden(row, root, shouldHide);
            }
            return;
        }
        QModelIndex root = fileView->rootIndex();
        for (int row = 0; row < model->rowCount(root); ++row) {
            QModelIndex index = model->index(row, 0, root);
            bool visible = model->fileName(index).contains(text, Qt::CaseInsensitive);
            fileView->setRowHidden(row, root, !visible);
        }
    });
    searchTimer->start(250);
}

void MainWindow::onTabCloseRequested(int index) {
    if (tabs->count() > 1) {
        QWidget *widget = tabs->widget(index);
        tabs->removeTab(index);
        widget->deleteLater();
    }
}

void MainWindow::onCurrentTabChanged(int index) {
    if (index < 0) return;
    fileView = qobject_cast<QTreeView *>(tabs->widget(index));

    if (fileView) {
        updateAddressBar();
        updateSelectionActions();
        updateStatusDetails();

        newFolderAction->setEnabled(true);
        newFileAction->setEnabled(true);
        sortAction->setEnabled(true);
        viewAction->setEnabled(true);
        upAction->setEnabled(true);
        terminalAction->setEnabled(true);
        hiddenFilesAction->setEnabled(true);
    } else {
        updateBreadcrumbs("computer://");
        if (statusLabel) statusLabel->setText(T("drives_overview"));
        if (statusSelectionLabel) statusSelectionLabel->setText("");

        copyAction->setEnabled(false);
        cutAction->setEnabled(false);
        deleteAction->setEnabled(false);
        renameAction->setEnabled(false);
        propertiesAction->setEnabled(false);
        newFolderAction->setEnabled(false);
        newFileAction->setEnabled(false);
        sortAction->setEnabled(false);
        viewAction->setEnabled(false);
        pasteAction->setEnabled(false);
        upAction->setEnabled(false);
        terminalAction->setEnabled(false);
        hiddenFilesAction->setEnabled(false);
    }
}

void MainWindow::navigateTo(const QString &path, bool addHistory) {
    if (!QDir(path).exists()) return;
    navigateTabTo(path);
    if (addHistory) {
        if (historyIndex + 1 < historyStack.size()) historyStack = historyStack.mid(0, historyIndex + 1);
        if (historyStack.isEmpty() || historyStack.last() != path) {
            historyStack.append(path);
            const int MAX_HISTORY = 100;
            if (historyStack.size() > MAX_HISTORY) {
                historyStack.removeFirst();
            }
        }
        historyIndex = historyStack.size() - 1;
    }
    backAction->setEnabled(historyIndex > 0);
    forwardAction->setEnabled(historyIndex + 1 < historyStack.size());
}

void MainWindow::navigateTabTo(const QString &path) {
    if (!QDir(path).exists()) return;

    if (!fileView) {
        int currentIndex = tabs->currentIndex();
        QWidget *oldTab = tabs->widget(currentIndex);

        openNewTab(path);
        if (tabs->count() > 1) {
            tabs->removeTab(tabs->indexOf(oldTab));
            oldTab->deleteLater();
        }
        return;
    }

    fileView->setRootIndex(model->index(path));
    updateAddressBar();
    int total = model->rowCount(fileView->rootIndex());
    if (statusLabel) statusLabel->setText(T("items_count").arg(total));
    if (statusSelectionLabel) statusSelectionLabel->setText("");
    int idx = tabs->indexOf(fileView);
    if (idx >= 0) {
        QString title = QFileInfo(path).fileName();
        if (title.isEmpty()) title = path;
        tabs->setTabText(idx, title);
        tabs->setTabIcon(idx, iconProvider.icon(QFileInfo(path)));
    }
}

void MainWindow::openNewTab(const QString &path) {
    const ThemeColors &c = AppStyle::colors();

    if (path == "computer://") {
        QWidget *container = new QWidget();
        container->setObjectName("computerContainer");
        container->setStyleSheet(QString("QWidget#computerContainer { background-color: %1; }").arg(c.surfaceBg));

        QVBoxLayout *mainLayout = new QVBoxLayout(container);
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setContentsMargins(30, 24, 30, 24);

        QLabel *header = new QLabel(T("drives_title"));
        header->setStyleSheet(QString("font-size: 17px; color: %1; font-weight: 700; margin-bottom: 6px; background: transparent;").arg(c.textPrimary));
        mainLayout->addWidget(header);

        QLabel *sub = new QLabel(T("drives_sub"));
        sub->setStyleSheet(QString("font-size: 12px; color: %1; margin-bottom: 12px; background: transparent;").arg(c.textSecondary));
        mainLayout->addWidget(sub);

        QGridLayout *grid = new QGridLayout();
        grid->setSpacing(16);
        grid->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        mainLayout->addLayout(grid);

        int row = 0, col = 0;
        for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
            if (!storage.isValid() || !storage.isReady()) continue;
            QString root = storage.rootPath();
            if (root != "/" && !root.startsWith("/media") && !root.startsWith("/mnt") && !root.startsWith("/run/media")) continue;

            QString name = storage.displayName().isEmpty()
                   ? T("local_disk") + " (" + root + ")"
                   : storage.displayName();
            if (storage.isRoot()) name = T("local_disk") + " (System)";

            DriveCard *card = new DriveCard(name, root, storage.bytesFree(), storage.bytesTotal(), [this](QString p){
                navigateTo(p);
            });

            grid->addWidget(card, row, col);
            col++;
            if (col > 2) { col = 0; row++; }
        }

        QScrollArea *scroll = new QScrollArea();
        scroll->setWidget(container);
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet(QString("QScrollArea { border: none; background-color: %1; }").arg(c.surfaceBg));

        int idx = tabs->addTab(scroll, QIcon(":/icons/computer.ico"), T("this_pc"));
        tabs->setCurrentIndex(idx);
        return;
    }

    QTreeView *view = new QTreeView;
    view->setModel(model);
    view->setRootIsDecorated(false);
    view->setItemsExpandable(false);
    view->setSortingEnabled(true);
    view->setAlternatingRowColors(false);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setIconSize(QSize(20, 20));
    view->setRootIndex(model->index(path));
    view->setDragDropMode(QAbstractItemView::DragDrop);
    view->setDefaultDropAction(Qt::MoveAction);

    view->header()->setStretchLastSection(false);
    view->header()->setSectionResizeMode(0, QHeaderView::Stretch);      // Name füllt Rest
    view->header()->setSectionResizeMode(1, QHeaderView::Interactive);  // Size
    view->header()->setSectionResizeMode(2, QHeaderView::Interactive);  // Type
    view->header()->setSectionResizeMode(3, QHeaderView::Interactive);  // Date

    view->setColumnWidth(1, 120);
    view->setColumnWidth(2, 200);
    view->setColumnWidth(3, 200);

    connect(view, &QTreeView::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(view, &QTreeView::doubleClicked, this, &MainWindow::onFileDoubleClicked);
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);

    QString title = QFileInfo(path).fileName();
    if (title.isEmpty()) title = path;
    int idx = tabs->addTab(view, iconProvider.icon(QFileInfo(path)), title);
    tabs->setCurrentIndex(idx);
    fileView = view;
}

void MainWindow::closeCurrentTab() {
    onTabCloseRequested(tabs->currentIndex());
}

void MainWindow::updateAddressBar() {
    if (!fileView || !fileView->rootIndex().isValid()) return;

    QString path = model->filePath(fileView->rootIndex());
    if (path.isEmpty()) {
        path = "Dieser PC";
    }
    updateBreadcrumbs(path);
}

void MainWindow::updateBreadcrumbs(const QString &path) {
    if (!breadcrumbLayout) return;
    const ThemeColors &c = AppStyle::colors();

    QLayoutItem *item;
    while ((item = breadcrumbLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    if (path == "computer://") {
        QPushButton *btn = new QPushButton("Dieser PC");
        btn->setFlat(true);
        breadcrumbLayout->addWidget(btn);
        return;
    }

    QString cleanPath = QDir::cleanPath(path);
    QStringList parts = cleanPath.split("/", Qt::SkipEmptyParts);

    if (parts.isEmpty()) {
        QPushButton *btn = new QPushButton("/");
        btn->setFlat(true);
        breadcrumbLayout->addWidget(btn);
        return;
    }

    QString currentPath = "";

    QPushButton *homeBtn = new QPushButton;
    homeBtn->setIcon(colorizeIcon(":/icons/home.svg", QColor(c.textPrimary)));
    homeBtn->setFlat(true);
    homeBtn->setCursor(Qt::PointingHandCursor);
    homeBtn->setToolTip(T("home"));
    connect(homeBtn, &QPushButton::clicked, this, [this] { navigateTo(QDir::homePath()); });
    breadcrumbLayout->addWidget(homeBtn);

    QLabel *sep1 = new QLabel("›");
    sep1->setObjectName("breadcrumbSep");
    breadcrumbLayout->addWidget(sep1);

    for (int i = 0; i < parts.size(); ++i) {
        currentPath += "/" + parts[i];
        QString partPath = currentPath;
        QPushButton *btn = new QPushButton(parts[i]);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, partPath] { navigateTo(partPath); });
        breadcrumbLayout->addWidget(btn);
        if (i < parts.size() - 1) {
            QLabel *sep = new QLabel("›");
            sep->setObjectName("breadcrumbSep");
            breadcrumbLayout->addWidget(sep);
        }
    }
    breadcrumbLayout->addStretch();
}

void MainWindow::updateSelectionActions() {
    if (!fileView || !fileView->selectionModel()) return;
    bool selected = !fileView->selectionModel()->selectedRows().isEmpty();
    copyAction->setEnabled(selected);
    cutAction->setEnabled(selected);
    deleteAction->setEnabled(selected);
    renameAction->setEnabled(selected);
    propertiesAction->setEnabled(selected);
    pasteAction->setEnabled(!clipboardPath.isEmpty());
}

void MainWindow::updateStatusDetails() {
    if (!statusSelectionLabel) return;
    if (!fileView || !fileView->selectionModel()) { statusSelectionLabel->setText(""); return; }
    QModelIndexList selected = fileView->selectionModel()->selectedRows();
    if (selected.isEmpty()) { statusSelectionLabel->setText(""); return; }

    if (selected.size() == 1) {
        QFileInfo info(model->filePath(selected.first()));
        if (info.isFile()) {
            statusSelectionLabel->setText(QString("1 Element ausgewählt · %1").arg(formatSize(info.size())));
            return;
        }
    }
    qint64 totalSize = 0;
    int fileCount = 0;
    for (const QModelIndex &idx : selected) {
        QFileInfo info(model->filePath(idx));
        if (info.isFile()) { totalSize += info.size(); fileCount++; }
    }
    if (fileCount > 0)
        statusSelectionLabel->setText(T("selected_n_size").arg(selected.size()).arg(formatSize(totalSize)));
    else
        statusSelectionLabel->setText(T("selected_n").arg(selected.size()));
}

void MainWindow::refreshCurrentView() {
    if (!fileView) return;
    QString path = model->filePath(fileView->rootIndex());
    fileView->setRootIndex(QModelIndex());
    fileView->setRootIndex(model->index(path));
    int total = model->rowCount(fileView->rootIndex());
    if (statusLabel) statusLabel->setText(T("items_count").arg(total));
    if (!trashPath.isEmpty() && trashItem) {
        bool full = QDir(trashPath).exists() && !QDir(trashPath).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
        trashItem->setIcon(QIcon(full ? ":/icons/trash_full.ico" : ":/icons/trash_empty.ico"));
    }
}

void MainWindow::showContextMenu(const QPoint &pos) {
    const ThemeColors &c = AppStyle::colors();
    QColor iconColor(c.textPrimary);

    QTreeView *view = qobject_cast<QTreeView *>(sender());
    if (!view) view = fileView;
    if (!view) return;
    QModelIndex index = view->indexAt(pos);
    QMenu menu(this);

    if (index.isValid()) {
        QString path = model->filePath(index);
        bool dir = model->isDir(index);
        QAction *open = menu.addAction(colorizeIcon(":/icons/open_folder.svg", iconColor), T("open"));
        menu.addSeparator();
        QAction *copy = menu.addAction(colorizeIcon(":/icons/copy.svg", iconColor), T("copy"));
        QAction *cut = menu.addAction(colorizeIcon(":/icons/cut.svg", iconColor), T("cut"));
        QAction *copyPath = menu.addAction(colorizeIcon(":/icons/link.svg", iconColor), T("copy_path"));
        QAction *pin = menu.addAction(QIcon(":/icons/pinquickaccess.ico"), T("pin_quick"));
        menu.addSeparator();
        QAction *rename = menu.addAction(colorizeIcon(":/icons/rename.svg", iconColor), T("rename"));
        QAction *remove = menu.addAction(colorizeIcon(":/icons/delete.svg", iconColor), T("delete"));
        menu.addSeparator();
        QAction *props = menu.addAction(colorizeIcon(":/icons/info.svg", iconColor), T("properties"));

        connect(open, &QAction::triggered, this, [this, path, dir] { if (dir) navigateTo(path); else QDesktopServices::openUrl(QUrl::fromLocalFile(path)); });
        connect(copy, &QAction::triggered, this, [this, path] { clipboardPath = path; isCut = false; updateSelectionActions(); });
        connect(cut, &QAction::triggered, this, [this, path] { clipboardPath = path; isCut = true; updateSelectionActions(); });
        connect(copyPath, &QAction::triggered, this, [path] { QApplication::clipboard()->setText(path); });
        connect(pin, &QAction::triggered, this, [this, path] {
            QStandardItem *item = new QStandardItem(QIcon(":/icons/pinquickaccess.ico"), QFileInfo(path).fileName());
            item->setData(path, Qt::UserRole);
            item->setEditable(false);
            sideModel->item(0)->appendRow(item);
        });
        connect(rename, &QAction::triggered, this, &MainWindow::renameSelected);
        connect(remove, &QAction::triggered, this, &MainWindow::deleteSelected);
        connect(props, &QAction::triggered, this, [this, path] {
            QFileInfo i(path);
            QString type = i.isDir() ? T("type_folder")
                                      : (i.suffix().isEmpty() ? T("type_file")
                                                               : i.suffix().toUpper() + "-" + T("type_file"));
            QString sizeStr = i.isDir() ? "-" : formatSize(i.size());
            ModernConfirmDialog dlg(T("properties"),
                QString("%1 %2\n%3 %4\n%5 %6\n%7 %8\n%9 %10")
                .arg(T("props_name"), i.fileName())
                .arg(T("props_path"), i.absoluteFilePath())
                .arg(T("props_type"), type)
                .arg(T("props_size"), sizeStr)
                .arg(T("props_modified"), i.lastModified().toString("dd.MM.yyyy hh:mm")), this, T("ok"), false);
            dlg.exec();
        });
    } else {
        QAction *paste = menu.addAction(colorizeIcon(":/icons/paste.svg", iconColor), T("paste"));
        menu.addSeparator();
        QAction *folder = menu.addAction(colorizeIcon(":/icons/new_folder.svg", iconColor), T("new_folder_ctx"));
        QAction *file = menu.addAction(colorizeIcon(":/icons/new_file.svg", iconColor), T("new_file_ctx"));
        menu.addSeparator();
        QAction *term = menu.addAction(colorizeIcon(":/icons/terminal.svg", iconColor), T("term_ctx"));
        paste->setEnabled(!clipboardPath.isEmpty());
        connect(paste, &QAction::triggered, this, &MainWindow::pasteHere);
        connect(folder, &QAction::triggered, this, &MainWindow::onNewFolderClicked);
        connect(file, &QAction::triggered, this, &MainWindow::createNewFile);
        connect(term, &QAction::triggered, this, &MainWindow::openInTerminal);
    }
    menu.exec(view->viewport()->mapToGlobal(pos));
}

void MainWindow::showSortMenu() {
    if (!fileView) return;
    QMenu menu(this);
    QAction *byName = menu.addAction(T("sort_name"));
    QAction *bySize = menu.addAction(T("sort_size"));
    QAction *byType = menu.addAction(T("sort_type"));
    QAction *byDate = menu.addAction(T("sort_date"));
    menu.addSeparator();
    QAction *asc = menu.addAction(T("sort_asc"));
    QAction *desc = menu.addAction(T("sort_desc"));
    asc->setCheckable(true);
    desc->setCheckable(true);
    QActionGroup *dirGroup = new QActionGroup(&menu);
    dirGroup->addAction(asc);
    dirGroup->addAction(desc);
    asc->setChecked(fileView->header()->sortIndicatorOrder() == Qt::AscendingOrder);
    desc->setChecked(fileView->header()->sortIndicatorOrder() == Qt::DescendingOrder);

    connect(byName, &QAction::triggered, this, [this] { fileView->sortByColumn(0, fileView->header()->sortIndicatorOrder()); });
    connect(bySize, &QAction::triggered, this, [this] { fileView->sortByColumn(1, fileView->header()->sortIndicatorOrder()); });
    connect(byType, &QAction::triggered, this, [this] { fileView->sortByColumn(2, fileView->header()->sortIndicatorOrder()); });
    connect(byDate, &QAction::triggered, this, [this] { fileView->sortByColumn(3, fileView->header()->sortIndicatorOrder()); });
    connect(asc, &QAction::triggered, this, [this] { fileView->sortByColumn(fileView->header()->sortIndicatorSection(), Qt::AscendingOrder); });
    connect(desc, &QAction::triggered, this, [this] { fileView->sortByColumn(fileView->header()->sortIndicatorSection(), Qt::DescendingOrder); });

    menu.exec(QCursor::pos());
}

void MainWindow::showViewMenu() {
    if (!fileView) return;
    QMenu menu(this);
    QAction *small = menu.addAction(T("view_small"));
    QAction *medium = menu.addAction(T("view_medium"));
    QAction *large = menu.addAction(T("view_large"));
    menu.addSeparator();
    QAction *hidden = menu.addAction(T("settings_hidden"));
    hidden->setCheckable(true);
    hidden->setChecked(showHidden);

    connect(small, &QAction::triggered, this, [this] { fileView->setIconSize(QSize(16, 16)); });
    connect(medium, &QAction::triggered, this, [this] { fileView->setIconSize(QSize(20, 20)); });
    connect(large, &QAction::triggered, this, [this] { fileView->setIconSize(QSize(32, 32)); });
    connect(hidden, &QAction::triggered, this, &MainWindow::toggleHiddenFiles);

    menu.exec(QCursor::pos());
}

void MainWindow::pasteHere() {
    if (clipboardPath.isEmpty() || !fileView) return;

    QString destDir = model->filePath(fileView->rootIndex());
    QString destPath = QDir(destDir).filePath(QFileInfo(clipboardPath).fileName());

    if (QFile::exists(destPath)) {
        ModernConfirmDialog dlg(T("overwrite_title"), T("overwrite_prompt"), this, T("overwrite"));
        if (dlg.exec() != QDialog::Accepted) return;
        if (!QFile::remove(destPath)) {
            ModernConfirmDialog dlg2(T("error"), T("overwrite_fail"), this, T("ok"), false);
            dlg2.exec();
            return;
        }
    }

    if (!QFile::copy(clipboardPath, destPath)) {
        ModernConfirmDialog dlg(T("error"), T("paste_fail"), this, T("ok"), false);
        dlg.exec();
        return;
    }

    if (isCut) {
        if (QFile::exists(clipboardPath)) {
            if (!QFile::remove(clipboardPath)) {
                ModernConfirmDialog dlg(T("error"), T("delete_orig_fail"), this, T("ok"), false);
                dlg.exec();
            }
        }
        clipboardPath.clear();
        isCut = false;
    }

    updateSelectionActions();
    refreshCurrentView();
}

void MainWindow::deleteSelected() {
    if (!fileView || !fileView->selectionModel()) return;
    QModelIndexList selected = fileView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QString message = T("delete_msg").arg(selected.size());

    ModernConfirmDialog dialog(T("delete_confirm"), message, this);

    if (dialog.exec() != QDialog::Accepted) return;

    int failCount = 0;
    for (const QModelIndex &index : selected) {
        QString path = model->filePath(index);
        bool success = false;
        if (model->isDir(index)) {
            success = QDir(path).removeRecursively();
        } else {
            success = QFile::remove(path);
        }
        if (!success) failCount++;
    }

    if (failCount > 0) {
        ModernConfirmDialog dlg(T("error"), T("delete_fail").arg(failCount), this, T("ok"), false);
        dlg.exec();
    }

    refreshCurrentView();
}

void MainWindow::renameSelected() {
    if (!fileView) return;
    QModelIndex index = fileView->currentIndex();
    if (!index.isValid()) return;
    QFileInfo info(model->filePath(index));
    bool ok = false;
    QString name = QInputDialog::getText(this, T("rename_dialog"), T("rename_label"), QLineEdit::Normal, info.fileName(), &ok);
    if (!ok || name.isEmpty() || name == info.fileName()) return;

    if (name.contains('/') || name.contains('\\') || name.contains("..")) {
        ModernConfirmDialog dlg(T("error"), T("invalid_chars"), this, T("ok"), false);
        dlg.exec();
        return;
    }

    if (!QFile::rename(info.absoluteFilePath(), info.dir().filePath(name))) {
        ModernConfirmDialog dlg(T("error"), T("rename_fail"), this, T("ok"), false);
        dlg.exec();
        return;
    }
    refreshCurrentView();
}

void MainWindow::copySelected() {
    if (!fileView) return;
    QModelIndex index = fileView->currentIndex();
    if (!index.isValid()) return;
    clipboardPath = model->filePath(index);
    isCut = false;
    updateSelectionActions();
}

void MainWindow::cutSelected() {
    if (!fileView) return;
    QModelIndex index = fileView->currentIndex();
    if (!index.isValid()) return;
    clipboardPath = model->filePath(index);
    isCut = true;
    updateSelectionActions();
}

void MainWindow::openSettings() {
    Updater updater;
    SettingsDialog dlg(this, &updater);

    dlg.onThemeChanged = [this](AppTheme theme) {
        AppStyle::setTheme(theme);
        applyTheme();
    };

    dlg.onLanguageChanged = [this](Language lang) {
        I18n::setLanguage(lang);
        // Toolbar-Texte aktualisieren
        if (backAction)        backAction->setText(T("back"));
        if (forwardAction)     forwardAction->setText(T("forward"));
        if (upAction)          upAction->setText(T("up"));
        if (refreshAction)     refreshAction->setText(T("refresh"));
        if (newFolderAction)   newFolderAction->setText(T("new_folder"));
        if (newFileAction)     newFileAction->setText(T("new_file"));
        if (cutAction)         cutAction->setText(T("cut"));
        if (copyAction)        copyAction->setText(T("copy"));
        if (pasteAction)       pasteAction->setText(T("paste"));
        if (renameAction)      renameAction->setText(T("rename"));
        if (deleteAction)      deleteAction->setText(T("delete"));
        if (sortAction)        sortAction->setText(T("sort"));
        if (viewAction)        viewAction->setText(T("view"));
        if (hiddenFilesAction) hiddenFilesAction->setText(T("hidden_files"));
        if (terminalAction)    terminalAction->setText(T("terminal"));
        if (propertiesAction)  propertiesAction->setText(T("properties"));
        if (settingsBtn)       settingsBtn->setText("⚙  " + T("settings"));
        if (statusLabel)       statusLabel->setText(T("ready"));
        // Sidebar neu aufbauen
        if (sideModel) {
            QStandardItem *quick = sideModel->item(0);
            if (quick) quick->setText(T("quick_access"));
            if (quick && quick->rowCount() >= 6) {
                quick->child(0)->setText(T("desktop"));
                quick->child(1)->setText(T("downloads"));
                quick->child(2)->setText(T("documents"));
                quick->child(3)->setText(T("pictures"));
                quick->child(4)->setText(T("music"));
                quick->child(5)->setText(T("videos"));
            }
            if (trashItem) trashItem->setText(T("trash"));
            QStandardItem *computer = sideModel->item(2);
            if (computer) computer->setText(T("this_pc"));
        }
        updateAddressBar();
    };

    connect(&dlg, &SettingsDialog::restartNeeded, &dlg, [this]() {
        ModernConfirmDialog ask(T("update_restart_title"), T("update_restart_msg"), this,
                                T("update_restart_now"), false);
        if (ask.exec() == QDialog::Accepted) {
            QProcess::startDetached(QCoreApplication::applicationFilePath(), {});
            qApp->quit();
        }
    });

    dlg.exec();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Fenstergröße und Position speichern
    SettingsManager::setWindowSize(size());
    SettingsManager::setWindowPosition(pos());
    SettingsManager::save();
    event->accept();
}

#include "mainwindow.moc"
