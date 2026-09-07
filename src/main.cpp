#include <QApplication>
#include "mainwindow/mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    MainWindow window;
    window.setWindowTitle("WinFilesPro");
    window.resize(1100, 700);
    window.show();
    
    return app.exec();
}