#include "mainwindow/mainwindow.h"
#include "mainwindow/style.h"
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

    MainWindow window;
    window.setWindowTitle("WinFilesPro");
    window.resize(1200, 760);
    window.show();

    return app.exec();
}
