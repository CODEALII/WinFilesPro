#include "mainwindow/mainwindow.h"
#include "mainwindow/style.h"
#include "mainwindow/settingsmanager.h"
#include "mainwindow/i18n.h"
#include <QApplication>
#include <QCoreApplication>
#include <QStyleFactory>
#include <QIcon>
#include <QDir>

int main(int argc, char *argv[]) {
    // Muss VOR dem Erzeugen von QApplication passieren: verhindert jegliche
    // Einmischung von Ubuntu/Yaru, GTK oder anderen Desktop-Themes.
    AppStyle::lockToCustomStyle();

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/explorer.ico"));

    // Sicherheitsnetz: falls Qt zur Laufzeit trotzdem versucht, ein
    // Plattform-Theme nachzuladen, erzwingen wir hier nochmal Fusion
    // und überschreiben die komplette Palette mit unseren eigenen Farben,
    // damit garantiert nichts vom System durchscheint.
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // Einstellungen laden
    SettingsManager::load();
    I18n::setLanguage(SettingsManager::getLanguage());
    AppStyle::setTheme(SettingsManager::getTheme());

    // Startpfad: optionales Argument (z.B. nach erneutem Start als root),
    // sonst der zuletzt besuchte Ordner
    QString startPath;
    const QStringList args = QCoreApplication::arguments();
    if (args.size() > 1 && QDir(args.at(1)).exists()) {
        startPath = args.at(1);
    } else {
        const QString last = SettingsManager::getLastPath();
        if (QDir(last).exists()) startPath = last;
    }

    MainWindow window;
    window.setWindowTitle("WinFilesPro");
    QSize windowSize = SettingsManager::getWindowSize();
    window.resize(windowSize);
    QPoint windowPos = SettingsManager::getWindowPosition();
    window.move(windowPos);
    if (!startPath.isEmpty()) window.openPath(startPath);
    window.show();

    return app.exec();
}
