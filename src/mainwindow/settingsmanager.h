#pragma once
#include <QString>
#include <QStringList>
#include <QSize>
#include <QPoint>
#include "i18n.h"
#include "style.h"
#include <QtGlobal>

class SettingsManager {
public:
    // Initialisierung beim Programmstart
    static void load();
    static void save();

    // Sprache
    static Language getLanguage();
    static void setLanguage(Language lang);

    // Theme
    static AppTheme getTheme();
    static void setTheme(AppTheme theme);

    // Fenstergröße und Position
    static QSize getWindowSize();
    static void setWindowSize(QSize size);
    static QPoint getWindowPosition();
    static void setWindowPosition(QPoint pos);

    // Letzter besuchter Pfad
    static QString getLastPath();
    static void setLastPath(const QString &path);

    // Versteckte Dateien
    static bool getShowHidden();
    static void setShowHidden(bool show);

    // Angepinnte Ordner im Schnellzugriff
    static QStringList getPinnedPaths();
    static void setPinnedPaths(const QStringList &paths);

    // Sortierung der Dateiliste
    static int getSortColumn();
    static void setSortColumn(int column);
    static Qt::SortOrder getSortOrder();
    static void setSortOrder(Qt::SortOrder order);

    // Allgemeine Optionen
    static bool getRestoreLastPath();
    static void setRestoreLastPath(bool v);
    static bool getMouseSideNav();
    static void setMouseSideNav(bool v);
    static bool getConfirmDelete();
    static void setConfirmDelete(bool v);
    static bool getAnimations();
    static void setAnimations(bool v);

    // Toolbar-Sichtbarkeit
    static bool getShowNavBar();
    static void setShowNavBar(bool v);
    static bool getShowCmdBar();
    static void setShowCmdBar(bool v);
    static bool getShowTabsBar();
    static void setShowTabsBar(bool v);

    // Größeneinheiten (0 = Windows KB/MB/GB, 1 = binär KiB/MiB, 2 = SI kB/MB)
    static int getSizeUnits();
    static void setSizeUnits(int v);

    // Bei Aktion (z.B. Convert to PNG) Quelldatei löschen
    static bool getDeleteAfterAction();
    static void setDeleteAfterAction(bool v);

    // Suchleiste ausblenden, nur mit Strg+F
    static bool getHideSearchBar();
    static void setHideSearchBar(bool v);

    // Dateityp-Zuordnungen (Endung -> Programm)
    static QString getAssoc(const QString &ext);
    static void setAssoc(const QString &ext, const QString &cmd);
    static void removeAssoc(const QString &ext);
    static QStringList assocExtensions();

    // Netzwerklaufwerke (zusätzliche Pfade)
    static QStringList getNetworkDrives();
    static void setNetworkDrives(const QStringList &drives);

private:
    static Language s_language;
    static AppTheme s_theme;
    static QSize s_windowSize;
    static QPoint s_windowPosition;
    static QString s_lastPath;
    static bool s_showHidden;
    static bool s_initialized;
};
