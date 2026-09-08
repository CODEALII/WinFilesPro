#include "settingsmanager.h"
#include <QSettings>
#include <QStandardPaths>

Language SettingsManager::s_language = Language::German;
AppTheme SettingsManager::s_theme = AppTheme::Dark;
QSize SettingsManager::s_windowSize = QSize(1200, 760);
QPoint SettingsManager::s_windowPosition = QPoint(100, 100);
QString SettingsManager::s_lastPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
bool SettingsManager::s_showHidden = false;
bool SettingsManager::s_initialized = false;

void SettingsManager::load() {
    QSettings settings("WinFilesPro", "WinFilesPro");

    // Sprache laden
    QString langStr = settings.value("language", "de").toString();
    s_language = (langStr == "en") ? Language::English : Language::German;

    // Theme laden
    QString themeStr = settings.value("theme", "dark").toString();
    s_theme = (themeStr == "light") ? AppTheme::Light : AppTheme::Dark;

    // Fenstergröße laden
    s_windowSize = settings.value("window/size", QSize(1200, 760)).toSize();

    // Fensterposition laden
    s_windowPosition = settings.value("window/position", QPoint(100, 100)).toPoint();

    // Letzter Pfad laden
    s_lastPath = settings.value("lastPath", QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).toString();

    // Versteckte Dateien laden
    s_showHidden = settings.value("showHidden", false).toBool();

    s_initialized = true;
}

void SettingsManager::save() {
    QSettings settings("WinFilesPro", "WinFilesPro");

    // Sprache speichern
    QString langStr = (s_language == Language::English) ? "en" : "de";
    settings.setValue("language", langStr);

    // Theme speichern
    QString themeStr = (s_theme == AppTheme::Light) ? "light" : "dark";
    settings.setValue("theme", themeStr);

    // Fenstergröße speichern
    settings.setValue("window/size", s_windowSize);

    // Fensterposition speichern
    settings.setValue("window/position", s_windowPosition);

    // Letzter Pfad speichern
    settings.setValue("lastPath", s_lastPath);

    // Versteckte Dateien speichern
    settings.setValue("showHidden", s_showHidden);

    settings.sync();
}

Language SettingsManager::getLanguage() {
    if (!s_initialized) load();
    return s_language;
}

void SettingsManager::setLanguage(Language lang) {
    s_language = lang;
}

AppTheme SettingsManager::getTheme() {
    if (!s_initialized) load();
    return s_theme;
}

void SettingsManager::setTheme(AppTheme theme) {
    s_theme = theme;
}

QSize SettingsManager::getWindowSize() {
    if (!s_initialized) load();
    return s_windowSize;
}

void SettingsManager::setWindowSize(QSize size) {
    s_windowSize = size;
}

QPoint SettingsManager::getWindowPosition() {
    if (!s_initialized) load();
    return s_windowPosition;
}

void SettingsManager::setWindowPosition(QPoint pos) {
    s_windowPosition = pos;
}

QString SettingsManager::getLastPath() {
    if (!s_initialized) load();
    return s_lastPath;
}

void SettingsManager::setLastPath(const QString &path) {
    s_lastPath = path;
}

bool SettingsManager::getShowHidden() {
    if (!s_initialized) load();
    return s_showHidden;
}

void SettingsManager::setShowHidden(bool show) {
    s_showHidden = show;
}

QStringList SettingsManager::getPinnedPaths() {
    QSettings settings("WinFilesPro", "WinFilesPro");
    return settings.value("pinnedPaths").toStringList();
}

void SettingsManager::setPinnedPaths(const QStringList &paths) {
    QSettings settings("WinFilesPro", "WinFilesPro");
    settings.setValue("pinnedPaths", paths);
    settings.sync();
}

int SettingsManager::getSortColumn() {
    QSettings settings("WinFilesPro", "WinFilesPro");
    return settings.value("sort/column", 0).toInt();
}

void SettingsManager::setSortColumn(int column) {
    QSettings settings("WinFilesPro", "WinFilesPro");
    settings.setValue("sort/column", column);
    settings.sync();
}

Qt::SortOrder SettingsManager::getSortOrder() {
    QSettings settings("WinFilesPro", "WinFilesPro");
    const int v = settings.value("sort/order", 0).toInt();
    return (v == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
}

void SettingsManager::setSortOrder(Qt::SortOrder order) {
    QSettings settings("WinFilesPro", "WinFilesPro");
    settings.setValue("sort/order", order == Qt::DescendingOrder ? 1 : 0);
    settings.sync();
}

bool SettingsManager::getRestoreLastPath() {
    QSettings settings("WinFilesPro", "WinFilesPro");
    return settings.value("opt/restoreLastPath", true).toBool();
}

void SettingsManager::setRestoreLastPath(bool v) {
    QSettings settings("WinFilesPro", "WinFilesPro");
    settings.setValue("opt/restoreLastPath", v);
    settings.sync();
}

bool SettingsManager::getMouseSideNav() {
    QSettings settings("WinFilesPro", "WinFilesPro");
    return settings.value("opt/mouseSideNav", true).toBool();
}

void SettingsManager::setMouseSideNav(bool v) {
    QSettings settings("WinFilesPro", "WinFilesPro");
    settings.setValue("opt/mouseSideNav", v);
    settings.sync();
}

bool SettingsManager::getConfirmDelete() {
    QSettings settings("WinFilesPro", "WinFilesPro");
    return settings.value("opt/confirmDelete", true).toBool();
}

void SettingsManager::setConfirmDelete(bool v) {
    QSettings settings("WinFilesPro", "WinFilesPro");
    settings.setValue("opt/confirmDelete", v);
    settings.sync();
}

bool SettingsManager::getAnimations() {
    QSettings settings("WinFilesPro", "WinFilesPro");
    return settings.value("opt/animations", true).toBool();
}

void SettingsManager::setAnimations(bool v) {
    QSettings settings("WinFilesPro", "WinFilesPro");
    settings.setValue("opt/animations", v);
    settings.sync();
}
