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

private:
    static Language s_language;
    static AppTheme s_theme;
    static QSize s_windowSize;
    static QPoint s_windowPosition;
    static QString s_lastPath;
    static bool s_showHidden;
    static bool s_initialized;
};
