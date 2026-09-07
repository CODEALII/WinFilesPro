#include "mainwindow.h"
#include <QApplication>
#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDateTime>
#include <QLabel>
#include <QHeaderView>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QStandardItemModel>
#include <QToolButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    // Windows Explorer Farben
    setStyleSheet(R"(
        QMainWindow {
            background-color: #ffffff;
        }
        QToolBar {
            background-color: #f0f0f0;
            border-bottom: 1px solid #cccccc;
            spacing: 2px;
            padding: 3px 4px;
        }
        QToolBar QToolButton {
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 2px;
            padding: 3px 8px;
            font-size: 12px;
            color: #000000;
        }
        QToolBar QToolButton:hover {
            background-color: #e5f3ff;
            border: 1px solid #cce4f7;
        }
        QToolBar QToolButton:pressed {
            background-color: #cce4f7;
            border: 1px solid #99caf7;
        }
        QToolBar QToolButton:disabled {
            color: #aaaaaa;
        }
        QTreeView {
            background-color: #ffffff;
            border: none;
            font-size: 13px;
            outline: none;
        }
        QTreeView::item {
            height: 22px;
            padding-left: 2px;
        }
        QTreeView::item:hover {
            background-color: #e5f3ff;
        }
        QTreeView::item:selected {
            background-color: #cce8ff;
            color: #000000;
        }
        QTreeView#sidebar {
            background-color: #f8f8f8;
            border-right: 1px solid #e0e0e0;
        }
        QTreeView#sidebar::item {
            height: 24px;
        }
        QLineEdit {
            border: 1px solid #cccccc;
            border-radius: 2px;
            padding: 3px 6px;
            background: white;
            font-size: 13px;
            selection-background-color: #cce8ff;
        }
        QLineEdit:focus {
            border: 1px solid #0078d7;
        }
        QStatusBar {
            background-color: #f0f0f0;
            border-top: 1px solid #cccccc;
            font-size: 12px;
            color: #444444;
        }
        QHeaderView::section {
            background-color: #f5f5f5;
            border: none;
            border-right: 1px solid #e0e0e0;
            border-bottom: 1px solid #e0e0e0;
            padding: 3px 8px;
            font-size: 12px;
        }
        QHeaderView::section:hover {
            background-color: #e5f3ff;
        }
        QSplitter::handle {
            background-color: #e0e0e0;
            width: 1px;
        }
        QMenu {
            background-color: #ffffff;
            border: 1px solid #cccccc;
            font-size: 13px;
        }
        QMenu::item {
            padding: 5px 24px;
        }
        QMenu::item:selected {
            background-color: #e5f3ff;
            color: #000000;
        }
        QMenu::separator {
            height: 1px;
            background-color: #e0e0e0;
            margin: 2px 0;
        }
    )");

    setupRibbon();
    setupLayout();
    setupStatusBar();
    navigateTo(QDir::homePath());
}

void MainWindow::setupRibbon() {
    // Nav Bar
    QToolBar *navBar = addToolBar("Navigation");
    navBar->setMovable(false);

    backAction    = navBar->addAction("◀");
    forwardAction = navBar->addAction("▶");
    upAction      = navBar->addAction("⬆");
    navBar->addSeparator();

    addressBar = new QLineEdit();
    addressBar->setMinimumWidth(400);
    navBar->addWidget(addressBar);
    navBar->addSeparator();

    QLineEdit *searchBar = new QLineEdit();
    searchBar->setPlaceholderText("🔍 Suchen...");
    searchBar->setFixedWidth(180);
    navBar->addWidget(searchBar);

    backAction->setEnabled(false);
    forwardAction->setEnabled(false);

    // Ribbon
    QToolBar *ribbon = addToolBar("Ribbon");
    ribbon->setMovable(false);

    cutAction        = ribbon->addAction("✂ Ausschneiden");
    copyAction       = ribbon->addAction("📋 Kopieren");
    pasteAction      = ribbon->addAction("📋 Einfügen");
    ribbon->addSeparator();
    deleteAction     = ribbon->addAction("🗑 Löschen");
    renameAction     = ribbon->addAction("✏ Umbenennen");
    ribbon->addSeparator();
    newFolderAction  = ribbon->addAction("📁 Neuer Ordner");
    ribbon->addSeparator();
    propertiesAction = ribbon->addAction("ℹ Eigenschaften");

    copyAction->setEnabled(false);
    cutAction->setEnabled(false);
    pasteAction->setEnabled(false);
    deleteAction->setEnabled(false);
    renameAction->setEnabled(false);
    propertiesAction->setEnabled(false);

    connect(backAction, &QAction::triggered, this, [this]() {
        if (historyIndex > 0) {
            historyIndex--;
            QString path = historyStack[historyIndex];
            fileView->setRootIndex(model->index(path));
            addressBar->setText(path);
            backAction->setEnabled(historyIndex > 0);
            forwardAction->setEnabled(true);
        }
    });

    connect(forwardAction, &QAction::triggered, this, [this]() {
        if (historyIndex < historyStack.size() - 1) {
            historyIndex++;
            QString path = historyStack[historyIndex];
            fileView->setRootIndex(model->index(path));
            addressBar->setText(path);
            forwardAction->setEnabled(historyIndex < historyStack.size() - 1);
            backAction->setEnabled(true);
        }
    });

    connect(upAction, &QAction::triggered, this, [this]() {
        QDir current(model->filePath(fileView->rootIndex()));
        if (current.cdUp()) navigateTo(current.absolutePath());
    });

    connect(addressBar, &QLineEdit::returnPressed, this, [this]() {
        navigateTo(addressBar->text());
    });

    connect(copyAction,      &QAction::triggered, this, &MainWindow::copySelected);
    connect(cutAction,       &QAction::triggered, this, &MainWindow::cutSelected);
    connect(pasteAction,     &QAction::triggered, this, &MainWindow::pasteHere);
    connect(deleteAction,    &QAction::triggered, this, &MainWindow::deleteSelected);
    connect(renameAction,    &QAction::triggered, this, &MainWindow::renameSelected);
    connect(propertiesAction,&QAction::triggered, this, [this]() {
        QModelIndex index = fileView->currentIndex();
        if (!index.isValid()) return;
        QFileInfo fi(model->filePath(index));
        QString info = QString("Name: %1\nPfad: %2\nGröße: %3 Bytes\nGeändert: %4\nTyp: %5")
            .arg(fi.fileName()).arg(fi.absolutePath()).arg(fi.size())
            .arg(fi.lastModified().toString("dd.MM.yyyy hh:mm"))
            .arg(fi.isDir() ? "Ordner" : fi.suffix().toUpper() + "-Datei");
        QMessageBox::information(this, "Eigenschaften", info);
    });
    connect(newFolderAction, &QAction::triggered, this, [this]() {
        QString currentPath = model->filePath(fileView->rootIndex());
        bool ok;
        QString name = QInputDialog::getText(
            this, "Neuer Ordner", "Ordnername:", QLineEdit::Normal, "Neuer Ordner", &ok);
        if (ok && !name.isEmpty()) {
            QDir(currentPath).mkdir(name);
            statusBar()->showMessage("Ordner erstellt: " + name);
        }
    });
}

void MainWindow::setupLayout() {
    model = new QFileSystemModel(this);
    model->setRootPath("/");

    // Sidebar mit eigenem simplen Model (nur Schnellzugriff + Laufwerke)
    QStandardItemModel *sideModel = new QStandardItemModel(this);

    // Schnellzugriff
    QStandardItem *quickAccess = new QStandardItem("⭐ Schnellzugriff");
    quickAccess->setEditable(false);
    quickAccess->setData("__header__", Qt::UserRole);

    struct QuickItem { QString label; QString path; };
    QList<QuickItem> quickItems = {
        {"🖥  Desktop",     QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)},
        {"⬇  Downloads",   QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)},
        {"📄 Dokumente",    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)},
        {"🖼  Bilder",      QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)},
        {"🎵 Musik",        QStandardPaths::writableLocation(QStandardPaths::MusicLocation)},
        {"🎬 Videos",       QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)},
    };

    for (auto &qi : quickItems) {
        QStandardItem *item = new QStandardItem(qi.label);
        item->setEditable(false);
        item->setData(qi.path, Qt::UserRole);
        quickAccess->appendRow(item);
    }
    sideModel->appendRow(quickAccess);

    // Laufwerke
    QStandardItem *drivesHeader = new QStandardItem("💻 Dieser PC");
    drivesHeader->setEditable(false);
    drivesHeader->setData("__header__", Qt::UserRole);

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (!storage.isValid() || !storage.isReady()) continue;
        QString mp = storage.rootPath();
        // Nur sinnvolle Mountpoints anzeigen
        if (mp == "/" || mp.startsWith("/media") || mp.startsWith("/mnt") || mp.startsWith("/run/media")) {
            QString label = storage.displayName().isEmpty()
                ? mp : storage.displayName() + " (" + mp + ")";
            // Größe anzeigen
            double totalGB = storage.bytesTotal() / 1e9;
            QString entry = QString("💾 %1  [%2 GB]").arg(label).arg(totalGB, 0, 'f', 0);
            QStandardItem *driveItem = new QStandardItem(entry);
            driveItem->setEditable(false);
            driveItem->setData(mp, Qt::UserRole);
            drivesHeader->appendRow(driveItem);
        }
    }
    sideModel->appendRow(drivesHeader);

    sidebar = new QTreeView();
    sidebar->setObjectName("sidebar");
    sidebar->setModel(sideModel);
    sidebar->expandAll();
    sidebar->setHeaderHidden(true);
    sidebar->setMinimumWidth(180);
    sidebar->setMaximumWidth(260);
    sidebar->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(sidebar, &QTreeView::clicked, this, [this, sideModel](const QModelIndex &index) {
        QString path = sideModel->itemFromIndex(index)->data(Qt::UserRole).toString();
        if (!path.isEmpty() && path != "__header__") navigateTo(path);
    });

    // Datei-Details-Ansicht
    fileView = new QTreeView();
    fileView->setModel(model);
    fileView->setItemsExpandable(false);
    fileView->setRootIsDecorated(false);
    fileView->setSortingEnabled(true);
    fileView->sortByColumn(0, Qt::AscendingOrder);
    fileView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    fileView->setContextMenuPolicy(Qt::CustomContextMenu);

    // Spalten — interaktiv resize + stretch
    QHeaderView *header = fileView->header();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(false);
    // Name nimmt den Rest, andere fixe Startbreiten
    header->resizeSection(0, 340);  // Name
    header->resizeSection(1, 130);  // Datum
    header->resizeSection(2, 90);   // Typ
    header->resizeSection(3, 90);   // Größe
    // Name stretcht wenn Fenster größer wird
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Interactive);

    connect(fileView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::showContextMenu);

    connect(fileView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &selected) {
        bool has = !selected.isEmpty();
        copyAction->setEnabled(has);
        cutAction->setEnabled(has);
        deleteAction->setEnabled(has);
        renameAction->setEnabled(has);
        propertiesAction->setEnabled(has);
        pasteAction->setEnabled(!clipboardPath.isEmpty());

        int count = fileView->selectionModel()->selectedRows().count();
        int total = model->rowCount(fileView->rootIndex());
        if (has)
            statusBar()->showMessage(QString("%1 von %2 Element(en) ausgewählt").arg(count).arg(total));
        else
            statusBar()->showMessage(QString("%1 Elemente").arg(total));
    });

    connect(fileView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        if (model->isDir(index)) navigateTo(model->filePath(index));
        else QDesktopServices::openUrl(QUrl::fromLocalFile(model->filePath(index)));
    });

    mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(sidebar);
    mainSplitter->addWidget(fileView);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setSizes({220, 880});

    setCentralWidget(mainSplitter);
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("Bereit");
}

void MainWindow::navigateTo(const QString &path) {
    if (!QDir(path).exists()) return;
    QModelIndex index = model->index(path);
    fileView->setRootIndex(index);
    addressBar->setText(path);

    if (historyIndex < historyStack.size() - 1)
        historyStack = historyStack.mid(0, historyIndex + 1);
    historyStack.append(path);
    historyIndex++;

    backAction->setEnabled(historyIndex > 0);
    forwardAction->setEnabled(false);

    int total = model->rowCount(index);
    statusBar()->showMessage(QString("%1 Elemente").arg(total));
}

void MainWindow::showContextMenu(const QPoint &pos) {
    QModelIndex index = fileView->indexAt(pos);
    QMenu menu(this);

    if (index.isValid()) {
        QString path  = model->filePath(index);
        bool    isDir = model->isDir(index);

        QAction *openAct   = menu.addAction("Öffnen");
        menu.addSeparator();
        QAction *copyAct   = menu.addAction("Kopieren");
        QAction *cutAct    = menu.addAction("Ausschneiden");
        menu.addSeparator();
        QAction *renameAct = menu.addAction("Umbenennen");
        QAction *deleteAct = menu.addAction("Löschen");
        menu.addSeparator();
        QAction *propsAct  = menu.addAction("Eigenschaften");

        connect(openAct,   &QAction::triggered, this, [this, path, isDir]() {
            if (isDir) navigateTo(path);
            else QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });
        connect(copyAct,   &QAction::triggered, this, [this, path]() {
            clipboardPath = path; isCut = false;
            pasteAction->setEnabled(true);
            statusBar()->showMessage("Kopiert: " + QFileInfo(path).fileName());
        });
        connect(cutAct,    &QAction::triggered, this, [this, path]() {
            clipboardPath = path; isCut = true;
            pasteAction->setEnabled(true);
            statusBar()->showMessage("Ausgeschnitten: " + QFileInfo(path).fileName());
        });
        connect(renameAct, &QAction::triggered, this, &MainWindow::renameSelected);
        connect(deleteAct, &QAction::triggered, this, &MainWindow::deleteSelected);
        connect(propsAct,  &QAction::triggered, this, [this, path]() {
            QFileInfo fi(path);
            QString info = QString("Name: %1\nPfad: %2\nGröße: %3 Bytes\nGeändert: %4\nTyp: %5")
                .arg(fi.fileName()).arg(fi.absolutePath()).arg(fi.size())
                .arg(fi.lastModified().toString("dd.MM.yyyy hh:mm"))
                .arg(fi.isDir() ? "Ordner" : fi.suffix().toUpper() + "-Datei");
            QMessageBox::information(this, "Eigenschaften", info);
        });
    } else {
        QAction *pasteAct     = menu.addAction("Einfügen");
        QAction *newFolderAct = menu.addAction("Neuer Ordner");
        pasteAct->setEnabled(!clipboardPath.isEmpty());

        connect(pasteAct,     &QAction::triggered, this, &MainWindow::pasteHere);
        connect(newFolderAct, &QAction::triggered, this, [this]() {
            QString currentPath = model->filePath(fileView->rootIndex());
            bool ok;
            QString name = QInputDialog::getText(
                this, "Neuer Ordner", "Ordnername:", QLineEdit::Normal, "Neuer Ordner", &ok);
            if (ok && !name.isEmpty()) {
                QDir(currentPath).mkdir(name);
                statusBar()->showMessage("Ordner erstellt: " + name);
            }
        });
    }

    menu.exec(fileView->viewport()->mapToGlobal(pos));
}

void MainWindow::pasteHere() {
    if (clipboardPath.isEmpty()) return;
    QString destDir  = model->filePath(fileView->rootIndex());
    QString fileName = QFileInfo(clipboardPath).fileName();
    QString destPath = destDir + "/" + fileName;
    if (QFile::copy(clipboardPath, destPath)) {
        if (isCut) { QFile::remove(clipboardPath); clipboardPath.clear(); isCut = false; }
        pasteAction->setEnabled(!clipboardPath.isEmpty());
        statusBar()->showMessage("Eingefügt: " + fileName);
    } else {
        QMessageBox::warning(this, "Fehler", "Einfügen fehlgeschlagen.");
    }
}

void MainWindow::deleteSelected() {
    QModelIndexList selected = fileView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    QString msg = selected.size() == 1
        ? "\"" + model->fileName(selected.first()) + "\" wirklich löschen?"
        : QString("%1 Elemente wirklich löschen?").arg(selected.size());
    if (QMessageBox::question(this, "Löschen", msg,
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    for (const QModelIndex &idx : selected) {
        QString path = model->filePath(idx);
        if (model->isDir(idx)) QDir(path).removeRecursively();
        else QFile::remove(path);
    }
    statusBar()->showMessage("Gelöscht");
}

void MainWindow::renameSelected() {
    QModelIndex index = fileView->currentIndex();
    if (!index.isValid()) return;
    QString path = model->filePath(index);
    QFileInfo fi(path);
    bool ok;
    QString newName = QInputDialog::getText(
        this, "Umbenennen", "Neuer Name:", QLineEdit::Normal, fi.fileName(), &ok);
    if (ok && !newName.isEmpty() && newName != fi.fileName()) {
        if (!QFile::rename(path, fi.dir().filePath(newName)))
            QMessageBox::warning(this, "Fehler", "Umbenennen fehlgeschlagen.");
        else statusBar()->showMessage("Umbenannt zu: " + newName);
    }
}

void MainWindow::copySelected() {
    QModelIndex index = fileView->currentIndex();
    if (!index.isValid()) return;
    clipboardPath = model->filePath(index);
    isCut = false;
    pasteAction->setEnabled(true);
    statusBar()->showMessage("Kopiert: " + model->fileName(index));
}

void MainWindow::cutSelected() {
    QModelIndex index = fileView->currentIndex();
    if (!index.isValid()) return;
    clipboardPath = model->filePath(index);
    isCut = true;
    pasteAction->setEnabled(true);
    statusBar()->showMessage("Ausgeschnitten: " + model->fileName(index));
}