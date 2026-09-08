#include "style.h"
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QFont>

AppTheme AppStyle::s_theme = AppTheme::Dark;

AppTheme AppStyle::currentTheme() { return s_theme; }
void AppStyle::setTheme(AppTheme theme) { s_theme = theme; }
void AppStyle::toggleTheme() {
    s_theme = (s_theme == AppTheme::Dark) ? AppTheme::Light : AppTheme::Dark;
}

const ThemeColors &AppStyle::colors() {
    static const ThemeColors dark {
        /*windowBg*/      "#1c1c1c",
        /*surfaceBg*/     "#1c1c1c",
        /*chromeBg*/      "#242424",
        /*sidebarBg*/     "#202020",
        /*elevatedBg*/    "#2b2b2b",

        /*border*/        "#3a3a3a",
        /*borderSubtle*/  "#2e2e2e",

        /*textPrimary*/   "#f2f2f2",
        /*textSecondary*/ "#a3a3a3",
        /*textDisabled*/  "#6e6e6e",

        /*accent*/        "#4cc2ff",
        /*accentHover*/   "#66cbff",
        /*accentPressed*/ "#2fa9e8",
        /*accentSubtle*/  "rgba(76, 194, 255, 0.16)",

        /*hoverBg*/       "#323232",
        /*pressedBg*/     "#2a2a2a",
        /*selectedBg*/    "rgba(76, 194, 255, 0.20)",
        /*selectedBorder*/"rgba(76, 194, 255, 0.55)",

        /*danger*/        "#e05252",
        /*dangerHover*/   "#f16767",
        /*warning*/       "#e8a33d",
        /*success*/       "#4fbf6a",

        /*scrollHandle*/      "#4a4a4a",
        /*scrollHandleHover*/ "#5f5f5f",
    };

    static const ThemeColors light {
        /*windowBg*/      "#fbfbfb",
        /*surfaceBg*/     "#ffffff",
        /*chromeBg*/      "#f3f3f3",
        /*sidebarBg*/     "#f3f3f3",
        /*elevatedBg*/    "#ffffff",

        /*border*/        "#e2e2e2",
        /*borderSubtle*/  "#ececec",

        /*textPrimary*/   "#1a1a1a",
        /*textSecondary*/ "#5f5f5f",
        /*textDisabled*/  "#b0b0b0",

        /*accent*/        "#0067c0",
        /*accentHover*/   "#1a7ad4",
        /*accentPressed*/ "#00548f",
        /*accentSubtle*/  "rgba(0, 103, 192, 0.10)",

        /*hoverBg*/       "#ececec",
        /*pressedBg*/     "#e0e0e0",
        /*selectedBg*/    "rgba(0, 103, 192, 0.12)",
        /*selectedBorder*/"rgba(0, 103, 192, 0.45)",

        /*danger*/        "#c42b1c",
        /*dangerHover*/   "#d63a2a",
        /*warning*/       "#9d5d00",
        /*success*/       "#0f7b3e",

        /*scrollHandle*/      "#c7c7c7",
        /*scrollHandleHover*/ "#a6a6a6",
    };

    return (s_theme == AppTheme::Dark) ? dark : light;
}

void AppStyle::lockToCustomStyle() {
    // WICHTIG: Diese Funktion läuft VOR dem Erzeugen von QApplication.
    // Sie verhindert, dass Qt irgendein Plattform-/Distributions-Theme
    // (GTK, Yaru, KDE/Kvantum, QGnomePlatform, etc.) lädt oder mischt.

    // 1. Keine Plattform-Theme-Bridge (das ist der Haupteinstiegspunkt für Yaru/GTK)
    qputenv("QT_QPA_PLATFORMTHEME", QByteArray());
    qunsetenv("QT_QPA_PLATFORMTHEME");

    // 2. Keinen Style-Override durch Umgebungsvariablen erlauben
    qunsetenv("QT_STYLE_OVERRIDE");

    // 3. GTK-Theme-Erkennung/-Integration explizit deaktivieren
    qunsetenv("QT_QPA_PLATFORM_PLUGIN_PATH");
    qputenv("GTK_THEME", QByteArray("")); // leer = kein GTK-Theme wird angefragt

    // 4. Icon-Theme-Fallbacks auf System-Icon-Themes verhindern
    //    (falls irgendwo QIcon::fromTheme() greifen würde)
    qunsetenv("QT_QPA_ICON_THEME"); // ungültiger Wert erzwingt eigene Icons statt Adwaita/Yaru

    // 5. Skalierung bewusst nicht vom Desktop übernehmen lassen (verhindert
    //    Größenschwankungen durch GNOME-Skalierungs-Presets)
    if (qEnvironmentVariableIsEmpty("QT_ENABLE_HIGHDPI_SCALING"))
        qputenv("QT_ENABLE_HIGHDPI_SCALING", QByteArray("1"));

    // 6. Immer den eingebauten "Fusion"-Style als Basis erzwingen. Fusion
    //    ist der einzige Qt-Style, der zu 100% durch eine QPalette und ein
    //    Stylesheet kontrollierbar ist und NICHTS vom System nachlädt.
    QApplication::setStyle(QStyleFactory::create("Fusion"));
}

void AppStyle::applyPalette() {
    const ThemeColors &c = colors();
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(c.windowBg));
    pal.setColor(QPalette::WindowText, QColor(c.textPrimary));
    pal.setColor(QPalette::Base, QColor(c.surfaceBg));
    pal.setColor(QPalette::AlternateBase, QColor(c.chromeBg));
    pal.setColor(QPalette::ToolTipBase, QColor(c.elevatedBg));
    pal.setColor(QPalette::ToolTipText, QColor(c.textPrimary));
    pal.setColor(QPalette::Text, QColor(c.textPrimary));
    pal.setColor(QPalette::Button, QColor(c.chromeBg));
    pal.setColor(QPalette::ButtonText, QColor(c.textPrimary));
    pal.setColor(QPalette::BrightText, QColor(c.danger));
    pal.setColor(QPalette::Link, QColor(c.accent));
    pal.setColor(QPalette::Highlight, QColor(c.selectedBg));
    pal.setColor(QPalette::HighlightedText, QColor(c.textPrimary));
    pal.setColor(QPalette::PlaceholderText, QColor(c.textSecondary));
    pal.setColor(QPalette::Disabled, QPalette::Text, QColor(c.textDisabled));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(c.textDisabled));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(c.textDisabled));
    pal.setColor(QPalette::Disabled, QPalette::Highlight, QColor(c.hoverBg));
    QApplication::setPalette(pal);
}

QString AppStyle::globalStyleSheet() {
    const ThemeColors &c = colors();

    return QString(R"(
        * {
            font-family: "Segoe UI", "Inter", "Noto Sans", sans-serif;
        }

        QMainWindow, QDialog {
            background-color: %windowBg;
        }

        /* Klickbare Elemente zeigen den Hand-Cursor */
        QPushButton, QToolButton, QComboBox, QCheckBox, QListWidget,
        QTreeView#sidebar::item, QMenu::item, QHeaderView::section {
            cursor: pointer;
        }

        QWidget#centralHost {
            background-color: %windowBg;
        }

        /* ---------- Toolbars (Ribbon-Ersatz) ---------- */
        QToolBar {
            background-color: %chromeBg;
            border: none;
            border-bottom: 1px solid %border;
            spacing: 4px;
            padding: 6px 10px;
        }
        QToolBar::separator {
            background-color: %border;
            width: 1px;
            margin: 6px 6px;
        }
        QToolBar QToolButton {
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 6px 10px;
            color: %textPrimary;
            font-size: 12.5px;
        }
        QToolBar QToolButton:hover {
            background-color: %hoverBg;
            cursor: pointer;
        }
        QToolBar QToolButton:pressed {
            background-color: %pressedBg;
        }
        QToolBar QToolButton:disabled {
            color: %textDisabled;
        }
        QToolBar QToolButton::menu-indicator { image: none; width: 0px; }

        /* ---------- Datei-/Ordneransicht ---------- */
        QTreeView {
            background-color: %surfaceBg;
            alternate-background-color: %surfaceBg;
            border: none;
            outline: none;
            font-size: 13px;
            color: %textPrimary;
            show-decoration-selected: 1;
        }
        QTreeView::item {
            height: 30px;
            padding-left: 4px;
            border: none;
        }
        QTreeView::item:hover {
            background-color: %hoverBg;
        }
        QTreeView::item:selected {
            background-color: %selectedBg;
            border: none;
            color: %textPrimary;
        }
        QTreeView::item:focus {
            outline: none;
            border: none;
        }
        QTreeView::branch { background: transparent; }

        /* ---------- Sidebar ---------- */
        QTreeView#sidebar {
            background-color: %sidebarBg;
            border: none;
            border-right: 1px solid %border;
            padding: 6px 0px;
        }
        QTreeView#sidebar::item {
            height: 34px;
            padding-left: 10px;
            border: none;
        }
        QTreeView#sidebar::item:hover {
            background-color: %hoverBg;
        }
        QTreeView#sidebar::item:selected {
            background-color: %selectedBg;
            color: %accent;
            font-weight: 600;
        }

        /* ---------- Adressleiste / Breadcrumbs ---------- */
        QWidget#breadcrumbWidget {
            background-color: %surfaceBg;
            border: 1px solid %border;
            border-radius: 6px;
        }
        QWidget#breadcrumbWidget QPushButton {
            background-color: transparent;
            border: none;
            border-radius: 4px;
            padding: 5px 8px;
            color: %textPrimary;
            font-size: 12.5px;
        }
        QWidget#breadcrumbWidget QPushButton:hover {
            background-color: %hoverBg;
        }
        QLabel#breadcrumbSep {
            color: %textSecondary;
        }

        /* ---------- Suchfeld ---------- */
        QLineEdit {
            background-color: %surfaceBg;
            border: 1px solid %border;
            border-radius: 6px;
            padding: 6px 10px;
            color: %textPrimary;
            font-size: 13px;
            selection-background-color: %accent;
        }
        QLineEdit:focus {
            border: 1px solid %accent;
        }
        QLineEdit:disabled {
            color: %textDisabled;
        }

        /* ---------- Tabs ---------- */
        QTabWidget::pane {
            border: none;
            top: 0px;
        }
        QTabWidget::tab-bar {
            left: 0px;
        }
        QTabBar {
            background-color: %chromeBg;
        }
        QTabBar::tab {
            background-color: %windowBg;
            color: %textSecondary;
            padding: 8px 14px;
            margin: 0px 2px 0px 0px;
            border: none;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            min-width: 160px;
            max-width: 220px;
        }
        QTabBar::tab:selected {
            background-color: %surfaceBg;
            color: %textPrimary;
        }
        QTabBar::tab:hover:!selected {
            background-color: %hoverBg;
        }
        QTabBar::close-button {
            image: none;
            background: transparent;
            border-radius: 4px;
            width: 16px; height: 16px;
            margin-left: 8px;
        }
        QTabBar::close-button:hover {
            background-color: %hoverBg;
        }
        QTabBar QToolButton {
            background-color: transparent;
            border: none;
            border-radius: 6px;
            padding: 4px;
        }
        QTabBar QToolButton:hover {
            background-color: %hoverBg;
        }

        /* ---------- Tabellenkopf ---------- */
        QHeaderView::section {
            background-color: %surfaceBg;
            border: none;
            border-bottom: 1px solid %border;
            border-right: 1px solid transparent;
            padding: 6px 10px;
            color: %textSecondary;
            font-size: 12px;
            font-weight: 600;
        }
        QHeaderView::section:hover {
            background-color: %hoverBg;
        }
        QHeaderView::down-arrow, QHeaderView::up-arrow {
            width: 8px; height: 8px;
        }

        QSplitter::handle {
            background-color: %border;
        }
        QSplitter::handle:horizontal { width: 1px; }
        QSplitter::handle:vertical { height: 1px; }

        /* ---------- Statusleiste ---------- */
        QStatusBar {
            background-color: %chromeBg;
            border-top: 1px solid %border;
            color: %textSecondary;
            font-size: 12px;
        }
        QStatusBar QLabel {
            color: %textSecondary;
            padding: 0 6px;
        }
        QStatusBar::item { border: none; }

        /* ---------- ComboBox ---------- */
        QComboBox {
            background-color: %surfaceBg;
            border: 1px solid %border;
            border-radius: 6px;
            padding: 5px 10px;
            color: %textPrimary;
        }
        QComboBox:hover { background-color: %hoverBg; }
        QComboBox::drop-down { border: none; width: 22px; }
        QComboBox QAbstractItemView {
            background-color: %elevatedBg;
            border: 1px solid %border;
            border-radius: 6px;
            selection-background-color: %selectedBg;
            color: %textPrimary;
            padding: 4px;
        }

        /* ---------- Menüs ---------- */
        QMenu {
            background-color: %elevatedBg;
            border: 1px solid %border;
            border-radius: 8px;
            padding: 6px;
        }
        QMenu::item {
            padding: 7px 28px 7px 14px;
            border-radius: 5px;
            color: %textPrimary;
            font-size: 13px;
        }
        QMenu::item:selected {
            background-color: %selectedBg;
            color: %textPrimary;
        }
        QMenu::item:disabled {
            color: %textDisabled;
        }
        QMenu::separator {
            height: 1px;
            background-color: %border;
            margin: 6px 8px;
        }
        QMenu::icon {
            padding-left: 6px;
        }

        /* ---------- Buttons (allgemein, z.B. in Dialogen) ---------- */
        QPushButton {
            background-color: %elevatedBg;
            border: 1px solid %border;
            border-radius: 6px;
            padding: 7px 16px;
            color: %textPrimary;
            font-size: 13px;
        }
        QPushButton:hover { background-color: %hoverBg; }
        QPushButton:pressed { background-color: %pressedBg; }
        QPushButton:disabled { color: %textDisabled; }
        QPushButton#primaryBtn {
            background-color: %accent;
            border: 1px solid %accent;
            color: #0b0b0b;
            font-weight: 600;
        }
        QPushButton#primaryBtn:hover { background-color: %accentHover; }
        QPushButton#primaryBtn:pressed { background-color: %accentPressed; }
        QPushButton#dangerBtn {
            background-color: %danger;
            border: 1px solid %danger;
            color: #ffffff;
            font-weight: 600;
        }
        QPushButton#dangerBtn:hover { background-color: %dangerHover; }

        /* ---------- Messagebox / Inputdialog (Fallback-Dialoge) ---------- */
        QMessageBox, QInputDialog {
            background-color: %elevatedBg;
        }
        QMessageBox QLabel, QInputDialog QLabel {
            color: %textPrimary;
        }

        /* ---------- Scrollbars ---------- */
        QScrollBar:vertical {
            background: transparent;
            width: 12px;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: %scrollHandle;
            border-radius: 5px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover { background: %scrollHandleHover; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }

        QScrollBar:horizontal {
            background: transparent;
            height: 12px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal {
            background: %scrollHandle;
            border-radius: 5px;
            min-width: 24px;
        }
        QScrollBar::handle:horizontal:hover { background: %scrollHandleHover; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }

        /* ---------- Tooltips ---------- */
        QToolTip {
            background-color: %elevatedBg;
            color: %textPrimary;
            border: 1px solid %border;
            border-radius: 4px;
            padding: 4px 8px;
            font-size: 12px;
        }

        /* ---------- ProgressBar (Laufwerkskarten) ---------- */
        QProgressBar {
            border: 1px solid %border;
            border-radius: 4px;
            background-color: %chromeBg;
        }
        QProgressBar::chunk {
            background-color: %accent;
            border-radius: 4px;
        }
        )")
        .replace("%windowBg", c.windowBg)
        .replace("%surfaceBg", c.surfaceBg)
        .replace("%chromeBg", c.chromeBg)
        .replace("%sidebarBg", c.sidebarBg)
        .replace("%elevatedBg", c.elevatedBg)
        .replace("%borderSubtle", c.borderSubtle)
        .replace("%border", c.border)
        .replace("%textPrimary", c.textPrimary)
        .replace("%textSecondary", c.textSecondary)
        .replace("%textDisabled", c.textDisabled)
        .replace("%accentHover", c.accentHover)
        .replace("%accentPressed", c.accentPressed)
        .replace("%accentSubtle", c.accentSubtle)
        .replace("%accent", c.accent)
        .replace("%hoverBg", c.hoverBg)
        .replace("%pressedBg", c.pressedBg)
        .replace("%selectedBg", c.selectedBg)
        .replace("%selectedBorder", c.selectedBorder)
        .replace("%dangerHover", c.dangerHover)
        .replace("%danger", c.danger)
        .replace("%warning", c.warning)
        .replace("%success", c.success)
        .replace("%scrollHandleHover", c.scrollHandleHover)
        .replace("%scrollHandle", c.scrollHandle);
}
