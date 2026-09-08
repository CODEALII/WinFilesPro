#include "mainwindow/mainwindow.h"
#include "mainwindow/style.h"
#include "mainwindow/settingsmanager.h"
#include "mainwindow/i18n.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    // Muss VOR dem Erzeugen von QApplication passieren: verhindert jegliche
    // Einmischung von Ubuntu/Yaru, GTK oder anderen Desktop-Themes.
    AppStyle::lockToCustomStyle();

    QApplication app(argc, argv);

    // Sicherheitsnetz: falls Qt zur Laufzeit trotzdem versucht, ein
    // Plattform-Theme nachzuladen, erzwingen wir hier nochmal Fusion
    // und überschreiben die komplette Palette mit unseren eigenen Farben,
    // damit garantiert nichts vom System durchscheint.
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // Einstellungen laden
    SettingsManager::load();
    I18n::setLanguage(SettingsManager::getLanguage());
    AppStyle::setTheme(SettingsManager::getTheme());

    MainWindow window;
    window.setWindowTitle("WinFilesPro");
    QSize windowSize = SettingsManager::getWindowSize();
    window.resize(windowSize);
    QPoint windowPos = SettingsManager::getWindowPosition();
    window.move(windowPos);
    window.show();

    return app.exec();
}
