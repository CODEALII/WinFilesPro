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
#include <QFileDialog>
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
#include <QProcessEnvironment>
#include <QUuid>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QPlainTextEdit>
#include <QTextBlock>

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
#include <QStyledItemDelegate>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDateTime>
#include <QUrlQuery>
#include <QTextStream>
#include <functional>
#include <cerrno>
#include <cstring>

#ifndef Q_OS_WIN32
#include <unistd.h>
#endif

// ---------------------------------------------------------------------------
// Hilfsfunktionen
// ---------------------------------------------------------------------------

// Rechtebündige Anzeige für die Spalte "Geändert am" mit rechtem Abstand
class DateColumnDelegate : public QStyledItemDelegate {
public:
    explicit DateColumnDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        if (index.column() == 3) {
            option->displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
            option->rect.adjust(0, 0, -10, 0);
        }
    }
};

// Linksbündige Anzeige für die Spalte "Größe"
class SizeColumnDelegate : public QStyledItemDelegate {
public:
    explicit SizeColumnDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        if (index.column() == 1) {
            option->displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
            option->rect.adjust(10, 0, 0, 0);
        }
    }
};

// Versteckte Dateien grau einfärben, sobald sie angezeigt werden
class HiddenStyleDelegate : public QStyledItemDelegate {
public:
    explicit HiddenStyleDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        if (index.column() != 0) return;
        const auto *m = qobject_cast<const QFileSystemModel *>(index.model());
        if (m && m->fileName(index).startsWith('.'))
            option->palette.setColor(QPalette::Text, QColor(AppStyle::colors().textSecondary));
    }
};

// Nur echte Datenträger anzeigen (echte Partitionen + Wechselmedien/Root),
// aber keine Pseudo-Dateisysteme (proc, tmpfs, snap-Loopdevices, ...).
static bool isUsableVolume(const QStorageInfo &storage) {
    if (!storage.isValid() || !storage.isReady()) return false;
    const QString root = storage.rootPath();
    if (root == "/") return true;
    const bool removable = root.startsWith("/media") || root.startsWith("/mnt") || root.startsWith("/run/media");
    if (removable) return true;
    const QString device = storage.device();
    if (!device.startsWith("/dev/")) return false;
    if (device.startsWith("/dev/loop")) return false;
    return true;
}

// Braucht der aktuelle Benutzer erhöhte Rechte, um den Ordner zu öffnen
// oder dort Dateien anzulegen? (nicht lesbar ODER nicht beschreibbar)
static bool needsElevation(const QString &path) {
#ifdef Q_OS_WIN32
    Q_UNUSED(path);
    return false;
#else
    if (::getuid() == 0) return false;
    if (!QFileInfo(path).exists()) return false;
    const QByteArray p = QFile::encodeName(path);
    if (::access(p.constData(), R_OK | X_OK) != 0) return true;
    if (::access(p.constData(), W_OK) != 0) return true;
    return false;
#endif
}

// Lesbare Fehlerbeschreibung für die zuletzt fehlgeschlagene Dateioperation
static QString curDirErrorReason() {
    return QString::fromLocal8Bit(std::strerror(errno));
}

// App mit polkit (OS-Passwortdialog) als root neu starten. Die aktuelle
// (nicht-elevierte) Instanz bleibt solange bestehen, bis die neue Instanz
// bestätigt, dass sie erfolgreich als root gestartet ist (Marker-Datei).
void MainWindow::launchElevated(const QString &path) {
    if (elevateProcess) return; // Läuft bereits eine Elevation
#ifndef Q_OS_WIN32
    const QString app = QCoreApplication::applicationFilePath();
    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    elevateMarker = QDir::tempPath() + QStringLiteral("/WinFilesPro-elevate-") + token + QStringLiteral(".marker");

    elevateProcess = new QProcess(this);
    elevateProcess->setProcessEnvironment(QProcessEnvironment::systemEnvironment());

    // pkexec leert die Umgebung weitgehend -> Display- und Session-Variablen
    // explizit an "env" durchreichen, damit die gestartete Instanz sichtbar ist.
    QStringList args{ QStringLiteral("env") };
    const char *envKeys[] = { "DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY", "XDG_RUNTIME_DIR",
                              "XDG_SESSION_TYPE", "DBUS_SESSION_BUS_ADDRESS",
                              "GDK_BACKEND", "QT_QPA_PLATFORM", "LANG", "LC_ALL", "HOME", "PATH" };
    for (const char *key : envKeys) {
        const QByteArray val = qgetenv(key);
        if (!val.isEmpty()) args << (QString(key) + '=' + QString::fromLocal8Bit(val));
    }
    args << app << QStringLiteral("--elevated") << path
         << QStringLiteral("--elevated-marker") << elevateMarker;

    elevateProcess->start(QStringLiteral("pkexec"), args);

    connect(elevateProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus) {
        stopElevationWait(); // Abgebrochen oder gescheitert: alte Instanz bleibt offen
    });

    elevatePoll = new QTimer(this);
    elevatePoll->setInterval(250);
    connect(elevatePoll, &QTimer::timeout, this, [this] {
        if (QFile::exists(elevateMarker)) {
            stopElevationWait();
            close();
        }
    });
    elevatePoll->start();
    QTimer::singleShot(90000, this, [this] { if (elevatePoll) stopElevationWait(); });
#endif
}

void MainWindow::stopElevationWait() {
    if (elevatePoll) { elevatePoll->stop(); elevatePoll->deleteLater(); elevatePoll = nullptr; }
    if (elevateProcess) { elevateProcess->deleteLater(); elevateProcess = nullptr; }
    if (!elevateMarker.isEmpty()) {
        QFile::remove(elevateMarker);
        elevateMarker.clear();
    }
}

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
    const int unit = SettingsManager::getSizeUnits();
    const double base = (unit == 2) ? 1000.0 : 1024.0;
    const double mb0 = base * base, gb0 = mb0 * base;
    QString kbS, mbS, gbS;
    if (unit == 0)      { kbS = " KB"; mbS = " MB"; gbS = " GB"; }
    else if (unit == 1) { kbS = " KiB"; mbS = " MiB"; gbS = " GiB"; }
    else                { kbS = " kB"; mbS = " MB"; gbS = " GB"; }
    if (bytes >= gb0) return QString::number(bytes / gb0, 'f', 2) + gbS;
    if (bytes >= mb0) return QString::number(bytes / mb0, 'f', 2) + mbS;
    if (bytes >= base) return QString::number(bytes / base, 'f', 1) + kbS;
    return QString::number(bytes) + " Bytes";
}

static QString itemsWithFreeSpace(int total, const QString &path) {
    QString text = T("items_count").arg(total);
    const QStorageInfo st(path);
    if (st.isValid() && st.isReady() && st.bytesTotal() > 0) {
        text += " · " + formatSize(st.bytesFree()) + " " + T("free");
    }
    return text;
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
// ModernInputDialog - minimale, moderne Eingabebox (ersetzt QInputDialog)
// ---------------------------------------------------------------------------
class ModernInputDialog : public QDialog {
public:
    ModernInputDialog(const QString &title, const QString &label, QWidget *parent = nullptr)
        : QDialog(parent), input(nullptr) {

        const ThemeColors &c = AppStyle::colors();

        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(420, 230);

        QWidget *container = new QWidget(this);
        container->setObjectName("inputContainer");
        container->setStyleSheet(QString(R"(
            QWidget#inputContainer {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 10px;
            }
            QLabel#inputTitle {
                color: %3;
                font-size: 15px;
                font-weight: 700;
                background: transparent;
                border: none;
            }
            QLabel#inputLabel {
                color: %4;
                font-size: 12.5px;
                background: transparent;
                border: none;
            }
        )").arg(c.elevatedBg, c.border, c.textPrimary, c.textSecondary));

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(18, 18, 18, 18);
        mainLayout->addWidget(container);

        QVBoxLayout *inner = new QVBoxLayout(container);
        inner->setContentsMargins(22, 20, 22, 18);
        inner->setSpacing(10);

        QLabel *titleLabel = new QLabel(title);
        titleLabel->setObjectName("inputTitle");
        inner->addWidget(titleLabel);

        QLabel *labelLabel = new QLabel(label);
        labelLabel->setObjectName("inputLabel");
        inner->addWidget(labelLabel);

        input = new QLineEdit;
        input->setFixedHeight(36);
        input->setObjectName("modernInput");
        input->setStyleSheet(QString(R"(
            QLineEdit#modernInput {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 6px;
                padding: 0 10px;
                color: %3;
                font-size: 13px;
            }
            QLineEdit#modernInput:focus { border: 1px solid %4; }
        )").arg(c.chromeBg, c.border, c.textPrimary, c.accent));
        inner->addWidget(input);

        inner->addStretch();

        QHBoxLayout *btns = new QHBoxLayout;
        QPushButton *cancel = new QPushButton(T("cancel"));
        QPushButton *okBtn = new QPushButton(T("ok"));
        okBtn->setObjectName("primaryBtn");
        btns->addStretch();
        btns->addWidget(cancel);
        btns->addWidget(okBtn);
        inner->addLayout(btns);

        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(okBtn, &QPushButton::clicked, this, [this] {
            if (!input->text().trimmed().isEmpty()) accept();
        });
        connect(input, &QLineEdit::returnPressed, okBtn, &QPushButton::click);

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(28);
        shadow->setColor(QColor(0, 0, 0, 90));
        shadow->setOffset(0, 6);
        container->setGraphicsEffect(shadow);

        setModal(true);
    }

    void setText(const QString &t) {
        input->setText(t);
        const int dot = t.lastIndexOf(QChar('.'));
        if (dot >= 0) input->setSelection(0, dot);
        else input->selectAll();
    }

    QString text() const { return input->text().trimmed(); }
    QLineEdit *input = nullptr;
};

static QString getModernText(QWidget *parent, const QString &title, const QString &label,
                             const QString &initial = QString(), bool *okPtr = nullptr) {
    ModernInputDialog dlg(title, label, parent);
    if (!initial.isEmpty()) dlg.setText(initial);
    const bool ok = (dlg.exec() == QDialog::Accepted);
    if (okPtr) *okPtr = ok;
    if (ok) return dlg.text();
    return QString();
}

// ---------------------------------------------------------------------------
// NewFileDialog - großes, modernes Fenster zum Erstellen neuer Dateien
// ---------------------------------------------------------------------------
class NewFileDialog : public QDialog {
public:
    explicit NewFileDialog(QWidget *parent = nullptr) : QDialog(parent), selectedExt("txt") {
        const ThemeColors &c = AppStyle::colors();

        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(720, 640);

        // Äußeres transparentes Layout für Schatten
        QVBoxLayout *outer = new QVBoxLayout(this);
        outer->setContentsMargins(16, 16, 16, 16);

        QWidget *container = new QWidget;
        container->setObjectName("newFileContainer");
        container->setStyleSheet(QString(R"(
            QWidget#newFileContainer {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 12px;
            }
        )").arg(c.elevatedBg, c.border));

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(42);
        shadow->setColor(QColor(0, 0, 0, 120));
        shadow->setOffset(0, 8);
        container->setGraphicsEffect(shadow);

        outer->addWidget(container);

        QVBoxLayout *layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Titelleiste
        QWidget *titleBar = new QWidget;
        titleBar->setStyleSheet(QString("background-color: %1; border-radius: 12px 12px 0 0; border-bottom: 1px solid %2;")
                                .arg(c.chromeBg, c.border));
        titleBar->setFixedHeight(52);
        QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(20, 0, 16, 0);
        QLabel *titleText = new QLabel(T("new_file_big_title"));
        titleText->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 700; background: transparent;").arg(c.textPrimary));
        QPushButton *closeBtn = new QPushButton("✕");
        closeBtn->setFixedSize(30, 30);
        closeBtn->setStyleSheet(QString(R"(
            QPushButton { background: transparent; border: none; border-radius: 6px; color: %1; font-size: 13px; }
            QPushButton:hover { background-color: %2; }
        )").arg(c.textSecondary, c.danger));
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
        titleLayout->addWidget(titleText);
        titleLayout->addStretch();
        titleLayout->addWidget(closeBtn);
        layout->addWidget(titleBar);

        // Suchfeld
        QWidget *searchRow = new QWidget;
        searchRow->setStyleSheet("background: transparent;");
        QHBoxLayout *searchLayout = new QHBoxLayout(searchRow);
        searchLayout->setContentsMargins(20, 12, 20, 4);
        QLineEdit *search = new QLineEdit;
        search->setPlaceholderText(T("new_file_search"));
        search->setClearButtonEnabled(true);
        search->setFixedHeight(36);
        search->setObjectName("newFileSearch");
        search->setStyleSheet(QString(R"(
            QLineEdit#newFileSearch {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 8px;
                padding: 0 12px;
                color: %3;
                font-size: 13px;
            }
            QLineEdit#newFileSearch:focus { border: 1px solid %4; }
        )").arg(c.chromeBg, c.border, c.textPrimary, c.accent));
        QAction *searchIcon = new QAction(QIcon(":/icons/search.ico"), "", search);
        search->addAction(searchIcon, QLineEdit::LeadingPosition);
        searchLayout->addWidget(search);
        layout->addWidget(searchRow);

        // Raster mit Vorlagen
        QWidget *gridHost = new QWidget;
        gridHost->setStyleSheet("background: transparent;");
        QGridLayout *grid = new QGridLayout(gridHost);
        grid->setContentsMargins(20, 8, 20, 8);
        grid->setSpacing(10);
        grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        int row = 0, col = 0;
        for (const TypeInfo &t : types()) {
            QToolButton *btn = new QToolButton;
            btn->setObjectName("typeTile");
            btn->setText(t.name);
            btn->setIcon(fileTypeIcon(t.shortLabel, t.color));
            btn->setIconSize(QSize(44, 44));
            btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            btn->setFixedSize(108, 84);
            btn->setAutoRaise(false);
            btn->setProperty("ext", t.ext);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(QString(R"(
                QToolButton#typeTile {
                    background: transparent;
                    border: 1px solid transparent;
                    border-radius: 10px;
                    color: %1;
                    font-size: 11.5px;
                    padding: 6px 4px;
                }
                QToolButton#typeTile:hover { background-color: %2; border-color: %3; }
            )").arg(c.textPrimary, c.hoverBg, c.border));
            connect(btn, &QToolButton::clicked, this, [this, btn] { selectType(btn); });

            grid->addWidget(btn, row, col);
            col++;
            if (col >= 5) { col = 0; row++; }
        }

        QScrollArea *scroll = new QScrollArea;
        scroll->setWidget(gridHost);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setStyleSheet(QString("QScrollArea { background: transparent; border: none; }"
                                      "QScrollArea > QWidget > QWidget { background: transparent; }"));
        allTiles = gridHost->findChildren<QToolButton *>();
        layout->addWidget(scroll, 1);

        // Suchfilter
        connect(search, &QLineEdit::textChanged, this, [this](const QString &text) {
            for (QToolButton *b : allTiles) {
                const bool show = text.isEmpty() || b->text().contains(text, Qt::CaseInsensitive)
                                 || b->property("ext").toString().contains(text, Qt::CaseInsensitive);
                b->setVisible(show);
            }
        });

        // Footer mit Dateiname und Aktionsbuttons
        QWidget *footer = new QWidget;
        footer->setStyleSheet(QString("background-color: %1; border-radius: 0 0 12px 12px; border-top: 1px solid %2;")
                              .arg(c.chromeBg, c.border));
        footer->setFixedHeight(82);
        QVBoxLayout *footerLayout = new QVBoxLayout(footer);
        footerLayout->setContentsMargins(20, 10, 20, 10);
        footerLayout->setSpacing(6);

        QHBoxLayout *nameRow = new QHBoxLayout;
        nameRow->setSpacing(10);
        QLabel *nameLabel = new QLabel(T("new_file_name_label"));
        nameLabel->setStyleSheet(QString("color: %1; font-size: 12.5px; font-weight: 600; background: transparent;").arg(c.textSecondary));
        nameEdit = new QLineEdit;
        nameEdit->setFixedHeight(34);
        nameEdit->setStyleSheet(QString(R"(
            QLineEdit { background-color: %1; border: 1px solid %2; border-radius: 6px; padding: 0 10px; color: %3; font-size: 13px; }
            QLineEdit:focus { border: 1px solid %4; }
        )").arg(c.elevatedBg, c.border, c.textPrimary, c.accent));
        nameEdit->setText(T("new_file_default_name"));
        nameRow->addWidget(nameLabel);
        nameRow->addWidget(nameEdit, 1);
        footerLayout->addLayout(nameRow);

        QHBoxLayout *btnRow = new QHBoxLayout;
        btnRow->setSpacing(10);
        QPushButton *cancel = new QPushButton(T("cancel"));
        cancel->setFixedHeight(34);
        cancel->setFixedWidth(100);
        QPushButton *create = new QPushButton(T("new_file_create"));
        create->setObjectName("primaryBtn");
        create->setFixedHeight(34);
        create->setFixedWidth(120);
        btnRow->addStretch();
        btnRow->addWidget(cancel);
        btnRow->addWidget(create);
        footerLayout->addLayout(btnRow);

        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(create, &QPushButton::clicked, this, &QDialog::accept);
        connect(nameEdit, &QLineEdit::returnPressed, create, &QPushButton::click);
        layout->addWidget(footer);
    }

    QString fileName() const {
        QString n = nameEdit->text().trimmed();
        if (n.isEmpty()) return QString();
        if (QFileInfo(n).suffix().isEmpty()) return n + "." + selectedExt;
        return n;
    }

private:
    struct TypeInfo {
        const char *name;
        const char *ext;
        const char *shortLabel;
        QColor color;
    };

    static QVector<TypeInfo> types() {
        const QColor gray("#7a869a"), blue("#4a90d9"), indigo("#5b6ad0"), green("#3fa66a"),
                     red("#d9564f"), purple("#9b59b6"), orange("#e08a3c"), yellow("#d9a62e"),
                     cyan("#2fb6bf"), pink("#d9637a"), teal("#2c9d8f");
        return {
            {"Text",           "txt",   "TXT", gray},
            {"Rich Text",      "rtf",   "RTF", gray},
            {"Markdown",       "md",    "MD",  gray},
            {"LaTeX",          "tex",   "TeX", gray},
            {"HTML",           "html",  "HTML", orange},
            {"CSS",            "css",   "CSS", blue},
            {"JavaScript",     "js",    "JS",  yellow},
            {"TypeScript",     "ts",    "TS",  blue},
            {"PHP",            "php",   "PHP", indigo},
            {"Python",         "py",    "PY",  blue},
            {"Ruby",           "rb",    "RUBY", red},
            {"Go",             "go",    "GO",  cyan},
            {"Rust",           "rs",    "RS",  orange},
            {"C",              "c",     "C",   blue},
            {"C++",            "cpp",   "C++", blue},
            {"Header",         "h",     "H",   gray},
            {"C#",             "cs",    "C#",  purple},
            {"Java",           "java",  "JAVA", orange},
            {"Kotlin",         "kt",    "KT",  purple},
            {"Swift",          "swift", "SWIFT", orange},
            {"Shell",          "sh",    "SH",  green},
            {"Batch",          "bat",   "BAT", gray},
            {"PowerShell",     "ps1",   "PS1", blue},
            {"SQL",            "sql",   "SQL", yellow},
            {"JSON",           "json",  "{}",  yellow},
            {"XML",            "xml",   "XML", blue},
            {"YAML",           "yaml",  "YAML", red},
            {"TOML",           "toml",  "TOML", gray},
            {"CSV",            "csv",   "CSV", green},
            {"INI",            "ini",   "INI", gray},
            {"PDF",            "pdf",   "PDF", red},
            {"Word",           "docx",  "DOCX", blue},
            {"Excel",          "xlsx",  "XLSX", green},
            {"PowerPoint",     "pptx",  "PPTX", orange},
            {"PNG",            "png",   "PNG", purple},
            {"JPG",            "jpg",   "JPG", purple},
            {"SVG",            "svg",   "SVG", orange},
            {"GIF",            "gif",   "GIF", pink},
            {"ICO",            "ico",   "ICO", gray},
            {"WebP",           "webp",  "WEBP", teal},
            {"MP3",            "mp3",   "MP3", pink},
            {"WAV",            "wav",   "WAV", cyan},
            {"FLAC",           "flac",  "FLAC", teal},
            {"OGG",            "ogg",   "OGG", purple},
            {"MP4",            "mp4",   "MP4", red},
            {"MKV",            "mkv",   "MKV", gray},
            {"AVI",            "avi",   "AVI", red},
            {"MOV",            "mov",   "MOV", teal},
            {"ZIP",            "zip",   "ZIP", yellow},
            {"7-Zip",          "7z",    "7Z",  gray},
            {"TAR",            "tar",   "TAR", gray},
        };
    }

    static QIcon fileTypeIcon(const QString &label, const QColor &color) {
        QPixmap pm(48, 48);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(color.darker(115));
        p.drawRoundedRect(QRectF(7, 7, 34, 34), 8, 8);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(4, 4, 34, 34), 8, 8);
        p.setPen(QColor(255, 255, 255));
        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(label.size() > 3 ? 13 : 16);
        p.setFont(f);
        QRectF textRect(4, 4, 34, 34);
        p.drawText(textRect, Qt::AlignCenter, label);
        p.end();
        return QIcon(pm);
    }

    void selectType(QToolButton *btn) {
        selectedExt = btn->property("ext").toString();
        const ThemeColors &c = AppStyle::colors();
        for (QToolButton *b : allTiles) {
            b->setStyleSheet(QString(R"(
                QToolButton#typeTile {
                    background: transparent;
                    border: 1px solid transparent;
                    border-radius: 10px;
                    color: %1;
                    font-size: 11.5px;
                    padding: 6px 4px;
                }
                QToolButton#typeTile:hover { background-color: %2; border-color: %3; }
            )").arg(c.textPrimary, c.hoverBg, c.border));
        }
        btn->setStyleSheet(QString(R"(
            QToolButton#typeTile {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 10px;
                color: %3;
                font-size: 11.5px;
                font-weight: 600;
                padding: 6px 4px;
            }
            QToolButton#typeTile:hover { background-color: %1; border-color: %2; }
        )").arg(AppStyle::colors().selectedBg, AppStyle::colors().selectedBorder, c.textPrimary));
        QString n = nameEdit->text();
        const int dot = n.lastIndexOf(QChar('.'));
        if (dot >= 0) n = n.left(dot);
        nameEdit->setText(n + "." + selectedExt);
        nameEdit->setFocus();
        nameEdit->selectAll();
    }

    QLineEdit *nameEdit = nullptr;
    QList<QToolButton *> allTiles;
    QString selectedExt;
};

// ---------------------------------------------------------------------------
// TextViewerDialog - moderner Text-Viewer mit Zeilennummern
// ---------------------------------------------------------------------------
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr) : QPlainTextEdit(parent) {
        lineArea = new LineNumberArea(this);
        connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineAreaWidth);
        connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineArea);
        connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::updateCursorLine);
        updateLineAreaWidth(0);
        highlightCurrentLine();
    }

    void lineNumberAreaPaintEvent(QPaintEvent *event) {
        const ThemeColors &c = AppStyle::colors();
        QPainter painter(lineArea);
        painter.fillRect(event->rect(), QColor(c.chromeBg));
        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + qRound(blockBoundingRect(block).height());
        painter.setPen(QColor(c.textSecondary));
        QFont f = font();
        f.setPixelSize(11);
        painter.setFont(f);
        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                const QString num = QString::number(blockNumber + 1);
                painter.drawText(0, top, lineArea->width() - 8, fontMetrics().height(),
                                 Qt::AlignRight, num);
            }
            block = block.next();
            top = bottom;
            bottom = top + qRound(blockBoundingRect(block).height());
            ++blockNumber;
        }
    }

    int lineNumberAreaWidth() const {
        const int digits = QString::number(std::max(1, blockCount())).length();
        return 16 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    }

private:
    class LineNumberArea : public QWidget {
    public:
        explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), editor(editor) {}
        QSize sizeHint() const override { return QSize(editor->lineNumberAreaWidth(), 0); }
    protected:
        void paintEvent(QPaintEvent *event) override { editor->lineNumberAreaPaintEvent(event); }
    private:
        CodeEditor *editor;
    };

    void updateLineAreaWidth(int) {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }

    void updateLineArea(const QRect &rect, int dy) {
        if (dy) lineArea->scroll(0, dy);
        else lineArea->update(0, rect.y(), lineArea->width(), rect.height());
        if (rect.contains(viewport()->rect())) updateLineAreaWidth(0);
    }

    void updateCursorLine() {
        QList<QTextEdit::ExtraSelection> extra;
        if (!isReadOnly()) {
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(QColor(255, 255, 255, 12));
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            sel.cursor = textCursor();
            sel.cursor.clearSelection();
            extra.append(sel);
        }
        setExtraSelections(extra);
    }

    void highlightCurrentLine() {}

    void resizeEvent(QResizeEvent *event) override {
        QPlainTextEdit::resizeEvent(event);
        const QRect cr = contentsRect();
        lineArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    }

    LineNumberArea *lineArea;
};

class TextViewerDialog : public QDialog {
public:
    explicit TextViewerDialog(const QString &filePath, QWidget *parent = nullptr)
        : QDialog(parent), filePath(filePath) {
        const ThemeColors &c = AppStyle::colors();

        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        resize(780, 580);

        QVBoxLayout *outer = new QVBoxLayout(this);
        outer->setContentsMargins(16, 16, 16, 16);

        QWidget *container = new QWidget;
        container->setObjectName("viewerContainer");
        container->setStyleSheet(QString(R"(
            QWidget#viewerContainer {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 12px;
            }
        )").arg(c.elevatedBg, c.border));

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(42);
        shadow->setColor(QColor(0, 0, 0, 120));
        shadow->setOffset(0, 8);
        container->setGraphicsEffect(shadow);

        outer->addWidget(container);

        QVBoxLayout *layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        QWidget *titleBar = new QWidget;
        titleBar->setStyleSheet(QString("background-color: %1; border-radius: 12px 12px 0 0; border-bottom: 1px solid %2;")
                                .arg(c.chromeBg, c.border));
        titleBar->setFixedHeight(52);
        QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(20, 0, 16, 0);
        QLabel *titleText = new QLabel(QFileInfo(filePath).fileName());
        titleText->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 700; background: transparent;").arg(c.textPrimary));
        titleLayout->addWidget(titleText);
        titleLayout->addStretch();
        QPushButton *copyBtn = new QPushButton(T("viewer_copy"));
        copyBtn->setCursor(Qt::PointingHandCursor);
        copyBtn->setStyleSheet(QString(R"(
            QPushButton { background-color: transparent; border: 1px solid %1; border-radius: 6px; color: %2; font-size: 12px; padding: 5px 12px; }
            QPushButton:hover { background-color: %3; }
        )").arg(c.border, c.textPrimary, c.hoverBg));
        connect(copyBtn, &QPushButton::clicked, this, [this] {
            if (editor) QApplication::clipboard()->setText(editor->toPlainText());
        });
        QPushButton *closeBtn = new QPushButton("✕");
        closeBtn->setFixedSize(30, 30);
        closeBtn->setStyleSheet(QString(R"(
            QPushButton { background: transparent; border: none; border-radius: 6px; color: %1; font-size: 13px; }
            QPushButton:hover { background-color: %2; }
        )").arg(c.textSecondary, c.danger));
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
        titleLayout->addWidget(copyBtn);
        titleLayout->addWidget(closeBtn);
        layout->addWidget(titleBar);

        QWidget *body = new QWidget;
        body->setStyleSheet("background: transparent;");
        QHBoxLayout *bodyLayout = new QHBoxLayout(body);
        bodyLayout->setContentsMargins(0, 12, 0, 0);

        editor = new CodeEditor;
        editor->setReadOnly(true);
        editor->setFrameShape(QFrame::NoFrame);
        editor->setLineWrapMode(QPlainTextEdit::NoWrap);
        editor->setFont(QFont(QStringLiteral("Noto Sans Mono"), 11));
        editor->setStyleSheet(QString(R"(
            QPlainTextEdit {
                background-color: %1;
                color: %2;
                border: none;
                selection-background-color: %3;
                selection-color: %4;
            }
        )").arg(c.surfaceBg, c.textPrimary, c.selectedBg, c.textPrimary));

        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            editor->setPlainText(QString::fromUtf8(f.readAll()));
            editor->moveCursor(QTextCursor::Start);
        }

        bodyLayout->addWidget(editor, 1);
        layout->addWidget(body, 1);

        QWidget *footer = new QWidget;
        footer->setStyleSheet(QString("background-color: %1; border-radius: 0 0 12px 12px; border-top: 1px solid %2;")
                              .arg(c.chromeBg, c.border));
        footer->setFixedHeight(44);
        QHBoxLayout *footerLayout = new QHBoxLayout(footer);
        footerLayout->setContentsMargins(20, 0, 20, 0);
        QLabel *status = new QLabel(T("viewer_lines")
            .arg(QString::number(editor->blockCount()))
            .arg(formatSize(QFileInfo(filePath).size())));
        status->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;").arg(c.textSecondary));
        footerLayout->addWidget(status);
        footerLayout->addStretch();
        layout->addWidget(footer);
    }

private:
    QString filePath;
    CodeEditor *editor = nullptr;
};

static bool isTextViewerType(const QFileInfo &info) {
    static const QStringList textExts = {
        "txt","md","rtf","tex","log","ini","cfg","conf","lst","csv",
        "json","xml","yaml","yml","toml","tsv",
        "html","htm","css","js","mjs","ts","jsx","tsx",
        "php","py","rb","pl","go","rs","sh","bash","zsh",
        "c","cpp","h","hpp","cc","cxx","cs","java","kt","swift",
        "sql","cmake","mk","makefile","qss","ui","svelte","vue",
        "bat","ps1","env","gitignore","gradle","properties","diff"
    };
    const QString ext = info.suffix().toLower();
    if (textExts.contains(ext)) return true;
    return info.fileName().toLower() == "makefile"
        || info.fileName().toLower() == "dockerfile"
        || info.fileName().toLower() == ".gitignore";
}

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

    // Maus-Seitentasten (Vor/Zurück) app-weit abfangen
    qApp->installEventFilter(this);

#ifndef Q_OS_WIN32
    // Relaunch als root: Zielpfad auslesen, Marker schreiben, damit die
    // alte (nicht-elevierte) Instanz sich sauber schließt.
    const QStringList args = QCoreApplication::arguments();
    const int elevIdx = args.indexOf(QStringLiteral("--elevated"));
    if (elevIdx >= 0 && elevIdx + 1 < args.size()) {
        const QString elevPath = args.at(elevIdx + 1);
        if (::getuid() == 0 && !elevPath.isEmpty()) {
            const int markerIdx = args.indexOf(QStringLiteral("--elevated-marker"));
            const QString marker = (markerIdx >= 0 && markerIdx + 1 < args.size())
                                       ? args.at(markerIdx + 1) : QString();
            if (!marker.isEmpty()) {
                QFile f(marker);
                if (f.open(QIODevice::WriteOnly)) f.close();
                QTimer::singleShot(10000, this, [marker] { QFile::remove(marker); });
            }
            elevatedStartPath = elevPath;
        }
    }
#endif

    applyTheme();

    setupRibbon();
    setupStatusBar();
    setupLayout();

    QString target = elevatedStartPath;
    if (target.isEmpty() && SettingsManager::getRestoreLastPath()) {
        const QString last = SettingsManager::getLastPath();
        if (QDir(last).exists()) target = last;
    }
    navigateTo(target.isEmpty() ? QDir::homePath() : target);
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (!windowShownOnce && SettingsManager::getAnimations()) {
        windowShownOnce = true;
        setWindowOpacity(0.0);
        QPropertyAnimation *fade = new QPropertyAnimation(this, "windowOpacity", this);
        fade->setDuration(180);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        windowShownOnce = true;
    }
}

void MainWindow::applyTheme() {
    AppStyle::applyPalette();
    qApp->setStyleSheet(AppStyle::globalStyleSheet());
}

void MainWindow::setupRibbon() {
    const ThemeColors &c = AppStyle::colors();
    QColor iconColor(c.textPrimary);

    navToolbar = addToolBar("Navigation");
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

    cmdToolbar = addToolBar("Befehle");
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
    new QShortcut(QKeySequence("Ctrl+Shift+D"), this, this, &MainWindow::duplicateCurrentTab);
    new QShortcut(QKeySequence("Alt+Left"), this, this, &MainWindow::onBackClicked);
    new QShortcut(QKeySequence("Alt+Right"), this, this, &MainWindow::onForwardClicked);
    new QShortcut(QKeySequence("Alt+Up"), this, this, &MainWindow::onUpClicked);
    new QShortcut(QKeySequence("Ctrl+L"), this, this, &MainWindow::startPathEdit);

    // Suchleiste: optional ausgeblendet, mit Strg+F einblenden
    searchBar->setVisible(!SettingsManager::getHideSearchBar());
    new QShortcut(QKeySequence::Find, this, [this]() {
        if (searchBar) {
            searchBar->setVisible(true);
            searchBar->setFocus();
            searchBar->selectAll();
        }
    });

    // Toolbar-Sichtbarkeit aus den Einstellungen übernehmen; das
    // eingebaute Toolbar-Kontextmenü wird entfernt, da diese Steuerung
    // bewusst über die Einstellungen läuft.
    navToolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    cmdToolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    navToolbar->setVisible(SettingsManager::getShowNavBar());
    cmdToolbar->setVisible(SettingsManager::getShowCmdBar());
}

void MainWindow::setupLayout() {
    model = new QFileSystemModel(this);
    model->setIconProvider(&iconProvider);
    model->setRootPath("/");
    showHidden = SettingsManager::getShowHidden();
    // Hidden immer im Filter lassen: Nur so bleiben auch Pfade durch
    // versteckte Zwischenordner (z.B. ~/.local für den Papierkorb) über
    // model->index() auflösbar. Die Sichtbarkeit versteckter Einträge wird
    // danach in den Ansichten per setRowHidden gesteuert.
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    // Neu geladene Einträge sofort ausblenden, wenn versteckte Dateien
    // nicht angezeigt werden sollen.
    connect(model, &QFileSystemModel::rowsInserted, this,
            [this](const QModelIndex &parent, int, int) {
        if (showHidden || !tabs) return;
        const auto views = tabs->findChildren<QTreeView *>();
        for (QTreeView *tv : views) {
            if (tv->rootIndex() == parent) applyHiddenPolicyToView(tv);
        }
    });

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

    // Angepinnte Ordner aus den Einstellungen laden
    pinnedPaths = SettingsManager::getPinnedPaths();
    for (const QString &p : pinnedPaths) {
        if (p.isEmpty() || !QFileInfo(p).isDir()) continue;
        bool exists = false;
        for (int r = 0; r < quick->rowCount(); ++r) {
            if (quick->child(r)->data(Qt::UserRole).toString() == p) { exists = true; break; }
        }
        if (exists) continue;
        QStandardItem *item = new QStandardItem(QIcon(":/icons/pinquickaccess.ico"), QFileInfo(p).fileName());
        item->setData(p, Qt::UserRole);
        item->setData(true, Qt::UserRole + 1);
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
        if (!isUsableVolume(storage)) continue;
        QString path = storage.rootPath();
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
    sidebar->setContextMenuPolicy(Qt::CustomContextMenu);
    sidebar->setAcceptDrops(true);
    sidebar->setDropIndicatorShown(true);
    sidebar->setDragDropMode(QAbstractItemView::DropOnly);
    sidebar->viewport()->installEventFilter(this);
    sidebar->expandAll();
    connect(sidebar, &QTreeView::clicked, this, &MainWindow::onSidebarClicked);
    connect(sidebar, &QTreeView::customContextMenuRequested, this, &MainWindow::showSidebarContextMenu);

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

    tabBarToolbar = new QToolBar("Tabs");
    tabBarToolbar->setObjectName("tabBarToolbar");
    tabBarToolbar->setMovable(false);
    tabBarToolbar->setFloatable(false);
    tabBarToolbar->setContextMenuPolicy(Qt::PreventContextMenu);
    tabBarToolbar->addWidget(topTabBarHost);
    addToolBar(Qt::TopToolBarArea, tabBarToolbar);
    insertToolBarBreak(tabBarToolbar);
    tabBarToolbar->setVisible(SettingsManager::getShowTabsBar());

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
        tabBtn->setProperty("tabIndex", index);
        tabBtn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(tabBtn, &QToolButton::clicked, this, [this, index] { tabs->setCurrentIndex(index); });
        connect(tabBtn, &QToolButton::customContextMenuRequested, this, &MainWindow::openTabContextMenu);
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
    const QString path = model->filePath(index);
    if (model->isDir(index)) navigateTo(path);
    else openFileWithAssoc(path);
}

void MainWindow::openFileWithAssoc(const QString &path) {
    const QString ext = QFileInfo(path).suffix().toLower();
    const QString app = SettingsManager::getAssoc(ext);
    if (!app.isEmpty()) { launchApp(app, path); return; }
    const QFileInfo info(path);
    if (isTextViewerType(info) && info.size() <= 8 * 1024 * 1024) {
        TextViewerDialog dlg(path, this);
        dlg.exec();
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::launchApp(const QString &app, const QString &file) {
    if (!QFile::exists(app)) {
        ModernConfirmDialog dlg(T("error"), T("app_not_found").arg(app), this, T("ok"), false);
        dlg.exec();
        return;
    }
    QProcess::startDetached(app, {file});
}

void MainWindow::convertToPng(const QString &path) {
    QString bin = QStandardPaths::findExecutable(QStringLiteral("magick"));
    if (bin.isEmpty()) bin = QStandardPaths::findExecutable(QStringLiteral("convert"));
    if (bin.isEmpty()) {
        ModernConfirmDialog dlg(T("error"), T("imagemagick_missing"), this, T("ok"), false);
        dlg.exec();
        return;
    }
    const QFileInfo in(path);
    const QString out = in.absolutePath() + "/" + in.completeBaseName() + ".png";
    auto *proc = new QProcess(this);
    proc->start(bin, {in.absoluteFilePath(), out});
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, proc, path, out](int code, QProcess::ExitStatus) {
        proc->deleteLater();
        if (code != 0) {
            ModernConfirmDialog dlg(T("error"), T("convert_fail"), this, T("ok"), false);
            dlg.exec();
            return;
        }
        if (SettingsManager::getDeleteAfterAction()) {
            if (!QFile::remove(path)) {
                ModernConfirmDialog dlg(T("error"), T("delete_orig_fail"), this, T("ok"), false);
                dlg.exec();
            }
        }
        refreshCurrentView();
    });
}

void MainWindow::onFileSelectionChanged() {
    updateSelectionActions();
    updateStatusDetails();
}

void MainWindow::onNewFolderClicked() {
    if (!fileView) return;
    bool ok = false;
    QString name = getModernText(this, T("folder_dialog"), T("folder_label"), T("new_folder"), &ok);
    if (!ok || name.isEmpty()) return;

    if (name.contains('/') || name.contains('\\') || name.contains("..")) {
        ModernConfirmDialog dlg(T("error"), T("invalid_chars"), this, T("ok"), false);
        dlg.exec();
        return;
    }

    if (!QDir(model->filePath(fileView->rootIndex())).mkdir(name)) {
        QString reason(curDirErrorReason());
        ModernConfirmDialog dlg(T("error"), T("folder_create_fail") + T("error_reason").arg(reason), this, T("ok"), false);
        dlg.exec();
        maybeShowAdminToast(model->filePath(fileView->rootIndex()));
        return;
    }
    refreshCurrentView();
}

void MainWindow::createNewFile() {
    if (!fileView) return;
    NewFileDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    QString name = dlg.fileName();
    if (name.isEmpty()) return;

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
        QString reason = f.errorString();
        if (reason.isEmpty()) reason = curDirErrorReason();
        ModernConfirmDialog dlg(T("error"), T("file_fail") + T("error_reason").arg(reason), this, T("ok"), false);
        dlg.exec();
        maybeShowAdminToast(model->filePath(fileView->rootIndex()));
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
    SettingsManager::setShowHidden(showHidden);
    applyHiddenPolicyToAllViews();
    if (fileView) onSearchTextChanged(searchBar ? searchBar->text() : QString());
}

bool MainWindow::rowHiddenByPolicy(const QModelIndex &index) const {
    return !showHidden && model->fileName(index).startsWith('.');
}

void MainWindow::applyHiddenPolicyToView(QTreeView *view) {
    if (!view) return;
    const QModelIndex root = view->rootIndex();
    if (!root.isValid()) return;
    const int rows = model->rowCount(root);
    for (int row = 0; row < rows; ++row) {
        const QModelIndex child = model->index(row, 0, root);
        if (model->fileName(child).startsWith('.'))
            view->setRowHidden(row, root, !showHidden);
    }
}

void MainWindow::applyHiddenPolicyToAllViews() {
    if (!tabs) return;
    const auto views = tabs->findChildren<QTreeView *>();
    for (QTreeView *tv : views) applyHiddenPolicyToView(tv);
}

QModelIndexList MainWindow::visibleSelectedRows() const {
    QModelIndexList all = fileView->selectionModel()->selectedRows();
    if (showHidden) return all;
    QModelIndexList out;
    for (const QModelIndex &idx : all)
        if (!model->fileName(idx).startsWith('.')) out << idx;
    return out;
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
            if (!showHidden && model->fileName(index).startsWith('.')) visible = false;
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
    refreshTopTabBar();
}

void MainWindow::onCurrentTabChanged(int index) {
    if (index < 0) return;
    refreshTopTabBar();
    fileView = qobject_cast<QTreeView *>(tabs->widget(index));
    applyHiddenPolicyToView(fileView);

    if (SettingsManager::getAnimations() && fileView) {
        QTreeView *tv = fileView;
        auto *eff = new QGraphicsOpacityEffect(tv);
        tv->setGraphicsEffect(eff);
        eff->setOpacity(0.0);
        QPropertyAnimation *fade = new QPropertyAnimation(eff, "opacity", tv);
        fade->setDuration(140);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        connect(fade, &QPropertyAnimation::finished, tv, [tv] { tv->setGraphicsEffect(nullptr); });
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    }

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
    applyHiddenPolicyToView(fileView);
    maybeShowAdminToast(path);
    updateAddressBar();
    int total = model->rowCount(fileView->rootIndex());
    if (!showHidden) {
        total = 0;
        for (int r = 0; r < model->rowCount(fileView->rootIndex()); ++r)
            if (!model->fileName(model->index(r, 0, fileView->rootIndex())).startsWith('.')) total++;
    }
    if (statusLabel) statusLabel->setText(itemsWithFreeSpace(total, path));
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
            if (!isUsableVolume(storage)) continue;
            QString root = storage.rootPath();

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

        // ---- Netzwerklaufwerke ----
        auto rebuildComputerTab = [this] {
            const int cur = tabs->currentIndex();
            tabs->removeTab(cur);
            openNewTab(QStringLiteral("computer://"));
        };

        QFrame *netSep = new QFrame;
        netSep->setFrameShape(QFrame::HLine);
        netSep->setStyleSheet(QString("color: %1;").arg(c.border));
        mainLayout->addSpacing(14);
        mainLayout->addWidget(netSep);
        mainLayout->addSpacing(10);

        QLabel *netHeader = new QLabel(T("network_drives_header"));
        netHeader->setStyleSheet(QString("font-size: 14px; color: %1; font-weight: 700; background: transparent;").arg(c.textPrimary));
        mainLayout->addWidget(netHeader);

        const QStringList netDrives = SettingsManager::getNetworkDrives();
        if (netDrives.isEmpty()) {
            QLabel *empty = new QLabel(T("network_no_drives"));
            empty->setStyleSheet(QString("font-size: 12px; color: %1; background: transparent;").arg(c.textSecondary));
            mainLayout->addWidget(empty);
        } else {
            auto openNetRow = [this](const QString &p) {
                if (p.contains(QStringLiteral("://")))
                    QDesktopServices::openUrl(QUrl(p));
                else if (QDir(p).exists())
                    navigateTo(p);
                else
                    QDesktopServices::openUrl(QUrl::fromLocalFile(p));
            };
            for (const QString &p : netDrives) {
                QString display = p;
                if (display.startsWith(QStringLiteral("/mnt/")) ||
                    display.startsWith(QStringLiteral("/media/")) ||
                    display.startsWith(QStringLiteral("/run/media/"))) {
                    display = QFileInfo(p).fileName();
                }
                QPushButton *row = new QPushButton;
                row->setCursor(Qt::PointingHandCursor);
                row->setStyleSheet(QString(R"(
                    QPushButton {
                        background: transparent;
                        border: 1px solid transparent;
                        border-radius: 6px;
                        color: %1;
                        font-size: 13px;
                        text-align: left;
                        padding: 7px 8px;
                    }
                    QPushButton:hover { background-color: %2; border-color: %3; }
                )").arg(c.textPrimary, c.hoverBg, c.border));
                row->setIcon(QIcon(":/icons/network.ico"));
                row->setIconSize(QSize(18, 18));
                row->setText(QString("%1  (%2)").arg(display, p));
                connect(row, &QPushButton::clicked, this, [this, p, openNetRow] { openNetRow(p); });

                QHBoxLayout *rowWrap = new QHBoxLayout;
                rowWrap->setContentsMargins(0, 0, 0, 0);
                QPushButton *remove = new QPushButton("×");
                remove->setFixedSize(26, 26);
                remove->setCursor(Qt::PointingHandCursor);
                remove->setToolTip(T("network_remove"));
                remove->setStyleSheet(QString(R"(
                    QPushButton { background: transparent; border: none; border-radius: 5px; color: %1; font-size: 15px; }
                    QPushButton:hover { background-color: %2; color: #ffffff; }
                )").arg(c.textSecondary, c.danger));
                connect(remove, &QPushButton::clicked, this, [this, p, rebuildComputerTab] {
                    QStringList upd = SettingsManager::getNetworkDrives();
                    upd.removeAll(p);
                    SettingsManager::setNetworkDrives(upd);
                    rebuildComputerTab();
                });
                rowWrap->addWidget(remove);

                QWidget *wrap = new QWidget;
                wrap->setLayout(rowWrap);
                QHBoxLayout *outer = new QHBoxLayout;
                outer->setContentsMargins(0, 0, 0, 0);
                outer->setSpacing(0);
                outer->addWidget(row, 1);
                outer->addWidget(wrap);
                mainLayout->addLayout(outer);
            }
        }

        QPushButton *addNet = new QPushButton(colorizeIcon(":/icons/network.ico", QColor(c.textPrimary)), T("network_add"));
        addNet->setCursor(Qt::PointingHandCursor);
        addNet->setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 8px;
                color: %3;
                font-size: 13px;
                padding: 8px 14px;
            }
            QPushButton:hover { background-color: %4; }
        )").arg(c.elevatedBg, c.border, c.textPrimary, c.hoverBg));
        connect(addNet, &QPushButton::clicked, this, [this, rebuildComputerTab] {
            bool ok = false;
            const QString drive = QInputDialog::getText(this, T("network_add"), T("network_path_label"),
                                                        QLineEdit::Normal, QStringLiteral("/mnt/nas"), &ok);
            if (!ok || drive.trimmed().isEmpty()) return;
            QString clean = drive.trimmed();
            QStringList upd = SettingsManager::getNetworkDrives();
            if (!upd.contains(clean)) upd << clean;
            SettingsManager::setNetworkDrives(upd);
            rebuildComputerTab();
        });
        mainLayout->addWidget(addNet);

        QScrollArea *scroll = new QScrollArea();
        scroll->setWidget(container);
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet(QString("QScrollArea { border: none; background-color: %1; }").arg(c.surfaceBg));

        int idx = tabs->addTab(scroll, QIcon(":/icons/computer.ico"), T("this_pc"));
        tabs->setCurrentIndex(idx);
        refreshTopTabBar();
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
    applyHiddenPolicyToView(view);
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
    view->setItemDelegateForColumn(1, new SizeColumnDelegate(view));
    view->setItemDelegateForColumn(3, new DateColumnDelegate(view));
    view->setItemDelegateForColumn(0, new HiddenStyleDelegate(view));

    // Gespeicherte Sortierung anwenden, Standard: Name aufsteigend
    const int sortCol = qBound(0, SettingsManager::getSortColumn(), 3);
    const Qt::SortOrder sortOrd = SettingsManager::getSortOrder();
    view->sortByColumn(sortCol, sortOrd);
    view->header()->setSortIndicator(sortCol, sortOrd);

    connect(view, &QTreeView::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(view, &QTreeView::doubleClicked, this, &MainWindow::onFileDoubleClicked);
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);

    maybeShowAdminToast(path);

    QString title = QFileInfo(path).fileName();
    if (title.isEmpty()) title = path;
    int idx = tabs->addTab(view, iconProvider.icon(QFileInfo(path)), title);
    tabs->setCurrentIndex(idx);
    fileView = view;
    refreshTopTabBar();
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

    // Bleistift-Button: wechselt in den Bearbeitungsmodus der Adressleiste
    auto addEditBtn = [this, &c] {
        QPushButton *editBtn = new QPushButton;
        editBtn->setIcon(colorizeIcon(":/icons/rename.svg", QColor(c.textSecondary)));
        editBtn->setFlat(true);
        editBtn->setCursor(Qt::PointingHandCursor);
        editBtn->setToolTip(QStringLiteral("Strg+L"));
        editBtn->setFixedSize(26, 26);
        editBtn->setStyleSheet(QString("QPushButton { border-radius: 5px; }"
                                       "QPushButton:hover { background-color: %1; }").arg(c.hoverBg));
        connect(editBtn, &QPushButton::clicked, this, &MainWindow::startPathEdit);
        breadcrumbLayout->addWidget(editBtn);
    };

    if (path == "computer://") {
        QPushButton *btn = new QPushButton("Dieser PC");
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setToolTip(T("this_pc"));
        breadcrumbLayout->addWidget(btn);
        breadcrumbLayout->addStretch();
        addEditBtn();
        return;
    }

    QString cleanPath = QDir::cleanPath(path);
    QStringList parts = cleanPath.split("/", Qt::SkipEmptyParts);

    if (parts.isEmpty()) {
        QPushButton *btn = new QPushButton("/");
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        breadcrumbLayout->addWidget(btn);
        breadcrumbLayout->addStretch();
        addEditBtn();
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
    addEditBtn();
}

void MainWindow::startPathEdit() {
    if (!fileView || !fileView->rootIndex().isValid()) return;
    if (!breadcrumbLayout) return;
    const QString path = model->filePath(fileView->rootIndex());

    QLayoutItem *item;
    while ((item = breadcrumbLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    pathEditor = new QLineEdit(path);
    pathEditor->setObjectName("pathEditor");
    pathEditor->setClearButtonEnabled(true);
    pathEditor->setMinimumWidth(360);
    pathEditor->installEventFilter(this);
    connect(pathEditor, &QLineEdit::returnPressed, this, [this] { commitPathEdit(pathEditor->text()); });
    breadcrumbLayout->addWidget(pathEditor, 1);

    pathEditor->setFocus(Qt::ShortcutFocusReason);
    pathEditor->selectAll();
}

void MainWindow::commitPathEdit(const QString &text) {
    QString p = text.trimmed();
    if (p.isEmpty()) { updateBreadcrumbs(model->filePath(fileView->rootIndex())); return; }
    if (!p.startsWith('/') && !p.startsWith("~")) p = QDir::homePath() + "/" + p;
    if (p.startsWith('~')) p = QDir::homePath() + p.mid(1);
    if (p == "computer://") {
        openNewTab("computer://");
        return;
    }
    if (QFileInfo::exists(QDir::cleanPath(p))) {
        navigateTo(QDir::cleanPath(p));
    } else {
        updateBreadcrumbs(model->filePath(fileView->rootIndex()));
    }
}

void MainWindow::updateSelectionActions() {
    if (!fileView || !fileView->selectionModel()) return;
    bool selected = !visibleSelectedRows().isEmpty();
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
    QModelIndexList selected = visibleSelectedRows();
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
    applyHiddenPolicyToView(fileView);
    int total = model->rowCount(fileView->rootIndex());
    if (statusLabel) statusLabel->setText(itemsWithFreeSpace(total, path));
    if (!trashPath.isEmpty() && trashItem) {
        bool full = QDir(trashPath).exists() && !QDir(trashPath).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
        trashItem->setIcon(QIcon(full ? ":/icons/trash_full.ico" : ":/icons/trash_empty.ico"));
    }
}

void MainWindow::openPath(const QString &path) {
    navigateTo(path);
}

void MainWindow::maybeShowAdminToast(const QString &path) {
    if (!needsElevation(path)) {
        if (adminToast) adminToast->hide();
        return;
    }
    adminToastPath = path;

    if (!adminToast) {
        const ThemeColors &c = AppStyle::colors();
        adminToast = new QWidget(this);
        adminToast->setObjectName("adminToast");
        adminToast->setStyleSheet(QString(R"(
            QWidget#adminToast {
                background-color: %1;
                border: 1px solid %3;
                border-radius: 10px;
            }
            QWidget#adminToast:hover { border-color: %5; }
            QLabel { background: transparent; border: none; }
            QLabel#toastTitle { color: %2; font-size: 13px; font-weight: 700; }
            QLabel#toastHint { color: %4; font-size: 11px; }
        )").arg(c.elevatedBg, c.textPrimary, c.border, c.textSecondary, c.accent));
        adminToast->setAttribute(Qt::WA_StyledBackground, true);
        adminToast->setAttribute(Qt::WA_Hover);
        adminToast->setCursor(Qt::PointingHandCursor);
        adminToast->installEventFilter(this);

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(adminToast);
        shadow->setBlurRadius(30);
        shadow->setColor(QColor(0, 0, 0, 110));
        shadow->setOffset(0, 4);
        adminToast->setGraphicsEffect(shadow);

        QHBoxLayout *lay = new QHBoxLayout(adminToast);
        lay->setContentsMargins(16, 12, 18, 12);
        lay->setSpacing(12);

        QLabel *icon = new QLabel(adminToast);
        icon->setPixmap(QIcon(":/icons/shield.ico").pixmap(24, 24));

        QLabel *title = new QLabel(T("admin_toast_title"), adminToast);
        title->setObjectName("toastTitle");
        QLabel *hint = new QLabel(T("admin_toast_open"), adminToast);
        hint->setObjectName("toastHint");

        QVBoxLayout *textLay = new QVBoxLayout;
        textLay->setSpacing(2);
        textLay->addWidget(title);
        textLay->addWidget(hint);

        lay->addWidget(icon);
        lay->addLayout(textLay);
    }

    updateAdminToastPosition();
    adminToast->show();
    adminToast->raise();
    if (SettingsManager::getAnimations()) {
        QPoint target = adminToast->pos();
        QPoint start = target + QPoint(0, 20);
        adminToast->move(start);
        QPropertyAnimation *slide = new QPropertyAnimation(adminToast, "pos", adminToast);
        slide->setDuration(220);
        slide->setEasingCurve(QEasingCurve::OutCubic);
        slide->setStartValue(start);
        slide->setEndValue(target);
        slide->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void MainWindow::updateAdminToastPosition() {
    if (!adminToast) return;
    adminToast->adjustSize();
    const int margin = 16;
    const int statusH = statusBar() ? statusBar()->height() : 0;
    const int y = height() - statusH - adminToast->height() - margin;
    adminToast->move(width() - adminToast->width() - margin, qMax(0, y));
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    updateAdminToastPosition();
}

void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);
    updateAdminToastPosition();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // Escape in der Adressleiste: Bearbeitung abbrechen
    if (obj == pathEditor && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            updateBreadcrumbs(model->filePath(fileView->rootIndex()));
            return true;
        }
    }

    // Maus-Seitentasten: Vor- und Zurücknavigation
    if (event->type() == QEvent::MouseButtonRelease &&
        SettingsManager::getMouseSideNav()) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::BackButton) {
            if (me->modifiers() == Qt::NoModifier) onBackClicked();
            return true;
        }
        if (me->button() == Qt::ForwardButton) {
            if (me->modifiers() == Qt::NoModifier) onForwardClicked();
            return true;
        }
    }

    if (obj == adminToast) {
        if (event->type() == QEvent::MouseButtonRelease) {
            launchElevated(adminToastPath);
            if (adminToast) adminToast->hide();
            return true;
        }
    }
    if (sidebar && obj == sidebar->viewport()) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            auto *drop = static_cast<QDragEnterEvent *>(event);
            if (drop->mimeData()->hasUrls()) {
                drop->setDropAction(Qt::CopyAction);
                drop->accept();
                return true;
            }
        } else if (event->type() == QEvent::Drop) {
            auto *drop = static_cast<QDropEvent *>(event);
            if (drop->mimeData()->hasUrls()) {
                const QList<QUrl> urls = drop->mimeData()->urls();
                for (const QUrl &u : urls) {
                    const QString p = u.toLocalFile();
                    if (!p.isEmpty() && QFileInfo(p).isDir()) addPinnedItem(p);
                }
                drop->setDropAction(Qt::CopyAction);
                drop->accept();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::addPinnedItem(const QString &path) {
    if (!sideModel) return;
    QStandardItem *quick = sideModel->item(0);
    if (!quick) return;
    if (path.isEmpty() || !QFileInfo(path).isDir()) return;
    for (int r = 0; r < quick->rowCount(); ++r) {
        if (quick->child(r)->data(Qt::UserRole).toString() == path) return;
    }
    QStandardItem *item = new QStandardItem(QIcon(":/icons/pinquickaccess.ico"), QFileInfo(path).fileName());
    item->setData(path, Qt::UserRole);
    item->setData(true, Qt::UserRole + 1);
    item->setEditable(false);
    quick->appendRow(item);
    if (!pinnedPaths.contains(path)) pinnedPaths.append(path);
    SettingsManager::setPinnedPaths(pinnedPaths);
    sidebar->expandAll();
}

void MainWindow::showSidebarContextMenu(const QPoint &pos) {
    QModelIndex index = sidebar->indexAt(pos);
    if (!index.isValid()) return;
    QStandardItem *item = sideModel->itemFromIndex(index);
    if (!item) return;

    QMenu menu(this);

    // Papierkorb: leeren
    if (item == trashItem) {
        QAction *empty = menu.addAction(QIcon(":/icons/trash_full.ico"), T("empty_trash"));
        connect(empty, &QAction::triggered, this, [this] {
            if (trashPath.isEmpty()) return;
            ModernConfirmDialog ask(T("empty_trash_title"), T("empty_trash_msg"), this, T("empty_trash_confirm"), false);
            if (ask.exec() != QDialog::Accepted) return;
            for (const QFileInfo &fi : QDir(trashPath).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
                if (fi.isDir()) QDir(fi.absoluteFilePath()).removeRecursively();
                else QFile::remove(fi.absoluteFilePath());
            }
            const QString infoDir = QFileInfo(trashPath).dir().filePath("info");
            for (const QFileInfo &fi : QDir(infoDir).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
                QFile::remove(fi.absoluteFilePath());
            }
            if (trashItem) trashItem->setIcon(QIcon(":/icons/trash_empty.ico"));
            refreshCurrentView();
        });
        menu.exec(sidebar->viewport()->mapToGlobal(pos));
        return;
    }

    // Angepinnte Ordner: lösen
    if (!item->data(Qt::UserRole + 1).toBool()) return;

    const QString path = item->data(Qt::UserRole).toString();
    QAction *unpin = menu.addAction(T("unpin"));
    connect(unpin, &QAction::triggered, this, [this, item, path] {
        QStandardItem *parentItem = item->parent();
        if (parentItem) parentItem->removeRow(item->row());
        pinnedPaths.removeAll(path);
        SettingsManager::setPinnedPaths(pinnedPaths);
    });
    menu.exec(sidebar->viewport()->mapToGlobal(pos));
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
        bool isZip = !dir && QFileInfo(path).suffix().compare(QStringLiteral("zip"), Qt::CaseInsensitive) == 0;
        if (!view->selectionModel()->isSelected(index)) {
            view->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
        QAction *open = menu.addAction(colorizeIcon(":/icons/open_folder.svg", iconColor), T("open"));
        QAction *openTab = dir ? menu.addAction(QIcon(":/icons/folder_open.ico"), T("open_new_tab")) : nullptr;

        QMenu *openWithMenu = nullptr;
        const QString ext = QFileInfo(path).suffix().toLower();
        if (!dir && !ext.isEmpty()) {
            openWithMenu = menu.addMenu(colorizeIcon(":/icons/open_folder.svg", iconColor), T("open_with"));
            const QString assocApp = SettingsManager::getAssoc(ext);
            if (!assocApp.isEmpty()) {
                QAction *a = openWithMenu->addAction(QFileInfo(assocApp).fileName());
                connect(a, &QAction::triggered, this, [this, path, assocApp] { launchApp(assocApp, path); });
                openWithMenu->addSeparator();
            }
            QAction *choose = openWithMenu->addAction(T("open_with_choose"));
            QAction *rememberType = openWithMenu->addAction(T("remember_for_type").arg(ext));
            rememberType->setCheckable(true);
            rememberType->setChecked(!SettingsManager::getAssoc(ext).isEmpty());
            connect(choose, &QAction::triggered, this, [this, path, ext, rememberType] {
                const QString app = QFileDialog::getOpenFileName(this, T("open_with_choose"));
                if (app.isEmpty()) return;
                launchApp(app, path);
                if (rememberType->isChecked()) SettingsManager::setAssoc(ext, app);
            });
        }

        const QStringList imgExts = { "png","jpg","jpeg","bmp","gif","webp","tif","tiff","ico","avif" };
        if (!dir && imgExts.contains(ext)) {
            QMenu *actions = menu.addMenu(QIcon(":/icons/image.ico"), T("acts_actions"));
            QAction *conv = actions->addAction(T("convert_png"));
            connect(conv, &QAction::triggered, this, [this, path] { convertToPng(path); });
        }
        menu.addSeparator();
        QAction *copy = menu.addAction(colorizeIcon(":/icons/copy.svg", iconColor), T("copy"));
        QAction *cut = menu.addAction(colorizeIcon(":/icons/cut.svg", iconColor), T("cut"));
        QAction *copyPath = menu.addAction(colorizeIcon(":/icons/link.svg", iconColor), T("copy_path"));
        QAction *pin = menu.addAction(QIcon(":/icons/pinquickaccess.ico"), T("pin_quick"));
        menu.addSeparator();
        QAction *rename = menu.addAction(colorizeIcon(":/icons/rename.svg", iconColor), T("rename"));
        QAction *remove = menu.addAction(colorizeIcon(":/icons/delete.svg", iconColor), T("delete"));
        QAction *compress = menu.addAction(QIcon(":/icons/zip.ico"), T("compress"));
        QAction *extract = isZip ? menu.addAction(QIcon(":/icons/zip.ico"), T("extract_here")) : nullptr;
        menu.addSeparator();
        QAction *props = menu.addAction(colorizeIcon(":/icons/info.svg", iconColor), T("properties"));

        connect(open, &QAction::triggered, this, [this, path, dir] { if (dir) navigateTo(path); else openFileWithAssoc(path); });
        if (openTab) connect(openTab, &QAction::triggered, this, [this, path] { openNewTab(path); });
        connect(copy, &QAction::triggered, this, [this, path] { clipboardPath = path; isCut = false; updateSelectionActions(); });
        connect(cut, &QAction::triggered, this, [this, path] { clipboardPath = path; isCut = true; updateSelectionActions(); });
        connect(copyPath, &QAction::triggered, this, [path] { QApplication::clipboard()->setText(path); });
        connect(pin, &QAction::triggered, this, [this, path] {
            addPinnedItem(path);
        });
        connect(rename, &QAction::triggered, this, &MainWindow::renameSelected);
        connect(remove, &QAction::triggered, this, &MainWindow::deleteSelected);
        connect(compress, &QAction::triggered, this, &MainWindow::createZip);
        if (extract) connect(extract, &QAction::triggered, this, &MainWindow::extractZip);
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

void MainWindow::duplicateCurrentTab() {
    if (!fileView) return;
    const QString path = model->filePath(fileView->rootIndex());
    openNewTab(path.isEmpty() ? QStringLiteral("computer://") : path);
}

void MainWindow::openTabContextMenu(const QPoint &pos) {
    auto *btn = qobject_cast<QToolButton *>(sender());
    if (!btn) return;
    const int index = btn->property("tabIndex").toInt();
    if (index < 0 || index >= tabs->count()) return;

    QMenu menu(this);
    QAction *dup = menu.addAction(T("duplicate_tab"));
    QAction *close = menu.addAction(T("close_tab"));

    connect(dup, &QAction::triggered, this, [this, index] {
        auto *v = qobject_cast<QTreeView *>(tabs->widget(index));
        if (v)
            openNewTab(model->filePath(v->rootIndex()));
        else
            openNewTab(QStringLiteral("computer://"));
    });
    connect(close, &QAction::triggered, this, [this, index] { onTabCloseRequested(index); });
    menu.exec(btn->mapToGlobal(pos));
}

void MainWindow::createZip() {
    if (!fileView || !fileView->selectionModel()) return;
    QModelIndexList sel = visibleSelectedRows();
    if (sel.isEmpty()) return;

    const QString zipBin = QStandardPaths::findExecutable(QStringLiteral("zip"));
    if (zipBin.isEmpty()) {
        ModernConfirmDialog dlg(T("error"), T("zip_missing"), this, T("ok"), false);
        dlg.exec();
        return;
    }

    QString current = model->filePath(fileView->rootIndex());
    QString baseName = model->fileName(sel.first());
    if (sel.size() > 1) baseName = QFileInfo(current).fileName();
    if (baseName.isEmpty()) baseName = QStringLiteral("archive");

    QStringList names;
    for (const QModelIndex &i : sel) names << model->fileName(i);

    QString out = QDir(current).filePath(baseName + ".zip");
    int counter = 1;
    while (QFile::exists(out))
        out = QDir(current).filePath(baseName + QString::number(counter++) + ".zip");

    QStringList args{ "-r", "-q", out };
    args += names;

    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(current);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, out](int code, QProcess::ExitStatus st) {
        proc->deleteLater();
        if (st == QProcess::NormalExit && code == 0) {
            if (statusLabel) statusLabel->setText(T("zip_done").arg(out));
            refreshCurrentView();
        } else {
            ModernConfirmDialog dlg(T("error"), T("zip_failed"), this, T("ok"), false);
            dlg.exec();
        }
    });
    proc->start(zipBin, args);
}

void MainWindow::extractZip() {
    if (!fileView || !fileView->selectionModel()) return;
    QModelIndexList sel = visibleSelectedRows();
    if (sel.size() != 1) return;

    const QString zipPath = model->filePath(sel.first());
    const QString unzipBin = QStandardPaths::findExecutable(QStringLiteral("unzip"));
    if (unzipBin.isEmpty()) {
        ModernConfirmDialog dlg(T("error"), T("zip_missing"), this, T("ok"), false);
        dlg.exec();
        return;
    }

    QString targetDir = QFileInfo(zipPath).dir().filePath(QFileInfo(zipPath).completeBaseName());
    QDir().mkpath(targetDir);

    auto *proc = new QProcess(this);
    proc->setWorkingDirectory(targetDir);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, targetDir](int code, QProcess::ExitStatus st) {
        proc->deleteLater();
        if (st == QProcess::NormalExit && code == 0) {
            if (statusLabel) statusLabel->setText(T("unzip_done").arg(targetDir));
            refreshCurrentView();
        } else {
            ModernConfirmDialog dlg(T("error"), T("zip_failed"), this, T("ok"), false);
            dlg.exec();
        }
    });
    proc->start(unzipBin, { "-o", "-q", zipPath });
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

    connect(byName, &QAction::triggered, this, [this] {
            fileView->sortByColumn(0, fileView->header()->sortIndicatorOrder());
            SettingsManager::setSortColumn(0);
        });
        connect(bySize, &QAction::triggered, this, [this] {
            fileView->sortByColumn(1, fileView->header()->sortIndicatorOrder());
            SettingsManager::setSortColumn(1);
        });
        connect(byType, &QAction::triggered, this, [this] {
            fileView->sortByColumn(2, fileView->header()->sortIndicatorOrder());
            SettingsManager::setSortColumn(2);
        });
        connect(byDate, &QAction::triggered, this, [this] {
            fileView->sortByColumn(3, fileView->header()->sortIndicatorOrder());
            SettingsManager::setSortColumn(3);
        });
        connect(asc, &QAction::triggered, this, [this] {
            fileView->sortByColumn(fileView->header()->sortIndicatorSection(), Qt::AscendingOrder);
            SettingsManager::setSortOrder(Qt::AscendingOrder);
        });
        connect(desc, &QAction::triggered, this, [this] {
            fileView->sortByColumn(fileView->header()->sortIndicatorSection(), Qt::DescendingOrder);
            SettingsManager::setSortOrder(Qt::DescendingOrder);
        });

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
    QModelIndexList selected = visibleSelectedRows();
    if (selected.isEmpty()) return;

    // Innerhalb des Papierkorbs löschen endgültig, statt erneut in den
    // Papierkorb zu verschieben.
    const QString currentDir = fileView->rootIndex().isValid()
            ? model->filePath(fileView->rootIndex()) : QString();
    const bool inTrash = !trashPath.isEmpty() && !currentDir.isEmpty()
            && QDir(currentDir).canonicalPath() == QDir(trashPath).canonicalPath();

    QString message = T(inTrash ? "delete_perm_msg" : "delete_msg").arg(selected.size());
    QString confirm = T(inTrash ? "delete_perm_confirm" : "delete_confirm");

    if (SettingsManager::getConfirmDelete()) {
        ModernConfirmDialog dialog(confirm, message, this);
        if (dialog.exec() != QDialog::Accepted) return;
    }

    // Kein Papierkorb verfügbar (z.B. Windows) oder im Papierkorb → endgültig löschen
    if (trashPath.isEmpty() || inTrash) {
        const QString failKey = T(inTrash ? "delete_perm_fail" : "delete_fail");
        int delFail = 0;
        for (const QModelIndex &index : selected) {
            QString path = model->filePath(index);
            bool success = model->isDir(index) ? QDir(path).removeRecursively()
                                               : QFile::remove(path);
            if (!success) delFail++;
        }
        if (delFail > 0) {
            ModernConfirmDialog dlg(T("error"), failKey.arg(delFail), this, T("ok"), false);
            dlg.exec();
        }
        if (inTrash && trashItem) {
            bool full = QDir(trashPath).exists()
                    && !QDir(trashPath).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
            trashItem->setIcon(QIcon(full ? ":/icons/trash_full.ico" : ":/icons/trash_empty.ico"));
        }
        refreshCurrentView();
        return;
    }

    QDir trashDir(trashPath);
    if (!trashDir.mkpath(".")) {
        ModernConfirmDialog dlg(T("error"), T("trash_unavailable"), this, T("ok"), false);
        dlg.exec();
        return;
    }
    const QDir infoDir(QFileInfo(trashPath).dir().filePath("info"));
    infoDir.mkpath(".");

    int failCount = 0;
    for (const QModelIndex &index : selected) {
        QString path = model->filePath(index);
        QFileInfo info(path);
        QString base = info.fileName();
        QString target = trashDir.filePath(base);
        int n = 1;
        while (QFileInfo::exists(target)) {
            target = trashDir.filePath(base + " (" + QString::number(n++) + ")");
        }

        if (QFile::rename(path, target)) {
            // Freedesktop-Trashinfo für Wiederherstellung/Historie anlegen
            QFile ti(infoDir.filePath(QFileInfo(target).fileName() + ".trashinfo"));
            if (ti.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream ts(&ti);
                ts << "[Trash Info]\n";
                ts << "Path=" << QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded) << "\n";
                ts << "DeletionDate=" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
            }
        } else {
            failCount++;
        }
    }

    // Papierkorb-Icon in der Sidebar aktualisieren
    if (trashItem) {
        bool full = QDir(trashPath).exists() && !QDir(trashPath).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
        trashItem->setIcon(QIcon(full ? ":/icons/trash_full.ico" : ":/icons/trash_empty.ico"));
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
    if (!index.isValid() || rowHiddenByPolicy(index)) return;
    QFileInfo info(model->filePath(index));
    bool ok = false;
    QString name = getModernText(this, T("rename_dialog"), T("rename_label"), info.fileName(), &ok);
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
    if (!index.isValid() || rowHiddenByPolicy(index)) return;
    clipboardPath = model->filePath(index);
    isCut = false;
    updateSelectionActions();
}

void MainWindow::cutSelected() {
    if (!fileView) return;
    QModelIndex index = fileView->currentIndex();
    if (!index.isValid() || rowHiddenByPolicy(index)) return;
    clipboardPath = model->filePath(index);
    isCut = true;
    updateSelectionActions();
}

void MainWindow::openSettings() {
    Updater updater;
    SettingsDialog dlg(this, &updater);

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

    connect(&dlg, &SettingsDialog::restartRequested, &dlg, [this]() {
        QProcess::startDetached(QCoreApplication::applicationFilePath(), {});
        qApp->quit();
    });

    if (dlg.exec() == QDialog::Accepted) {
        // Geänderte Sichtbarkeiten sofort übernehmen
        navToolbar->setVisible(SettingsManager::getShowNavBar());
        cmdToolbar->setVisible(SettingsManager::getShowCmdBar());
        if (tabBarToolbar) tabBarToolbar->setVisible(SettingsManager::getShowTabsBar());
        searchBar->setVisible(!SettingsManager::getHideSearchBar());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Fenstergröße, Position und zuletzt geöffneten Ordner speichern
    SettingsManager::setWindowSize(size());
    SettingsManager::setWindowPosition(pos());
    if (fileView && fileView->rootIndex().isValid()) {
        SettingsManager::setLastPath(model->filePath(fileView->rootIndex()));
    }
    SettingsManager::setShowHidden(showHidden);
    SettingsManager::save();
    event->accept();
}

#include "mainwindow.moc"
