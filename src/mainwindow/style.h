#pragma once
#include <QString>
#include <QColor>

// Zentrale Stelle für alles, was mit dem Look der App zu tun hat.
// Ziel: EIN einziges, konsistentes "Windows 11"-Design, das komplett
// unabhängig vom System-/Desktop-Theme (insbesondere Ubuntu/Yaru) ist.
// Nichts hier fragt jemals nach QApplication::palette() vom System oder
// nach GTK-Settings - alle Farben sind hart kodiert.

enum class AppTheme { Dark, Light };

struct ThemeColors {
    // Grundflächen
    QString windowBg;
    QString surfaceBg;      // Content-Bereich (Dateiliste)
    QString chromeBg;       // Toolbar / Statusbar / Tabbar
    QString sidebarBg;
    QString elevatedBg;     // Menüs, Dialoge, Popups

    // Ränder / Trenner
    QString border;
    QString borderSubtle;

    // Text
    QString textPrimary;
    QString textSecondary;
    QString textDisabled;

    // Akzent (Windows 11 Blau, in beiden Themes gleich)
    QString accent;
    QString accentHover;
    QString accentPressed;
    QString accentSubtle;   // z.B. Selektion-Hintergrund

    // Interaktionszustände (neutral, für Items/Buttons)
    QString hoverBg;
    QString pressedBg;
    QString selectedBg;
    QString selectedBorder;

    // Statusfarben
    QString danger;
    QString dangerHover;
    QString warning;
    QString success;

    QString scrollHandle;
    QString scrollHandleHover;
};

class AppStyle {
public:
    static AppTheme currentTheme();
    static void setTheme(AppTheme theme);
    static void toggleTheme();

    static const ThemeColors &colors();
    static QString globalStyleSheet();

    // Muss so früh wie möglich in main() aufgerufen werden, BEVOR QApplication
    // erzeugt wird, um jegliche Einmischung von Desktop-/GTK-Themes zu verhindern.
    static void lockToCustomStyle();

    // Schreibt eine vollständige, eigene QPalette auf QApplication, damit kein
    // Systemtheme (Yaru/GNOME) über Paletten-Rollen (Highlight, Base, ...)
    // durchscheinen kann. Wird nach dem Style-Setup in main() aufgerufen und
    // vom MainWindow bei jedem Theme-Wechsel erneut angewendet.
    static void applyPalette();

private:
    static AppTheme s_theme;
};
