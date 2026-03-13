#include "../Inc/mainwindow.h"
#include <QApplication>
#include <QTimer>
#include <QFontDatabase>
#include <cmath>

int main(int argc, char *argv[]) {
    // Initialize the application
    QApplication a(argc, argv);

    // Setup font
    int fontId = QFontDatabase::addApplicationFont(":/Fonts/FOT-NewRodin Pro M.otf");
    QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
    QFont defaultFont(fontFamily, 36);
    defaultFont.setKerning(false);
    QApplication::setFont(defaultFont);

    // Create the main window
    MainWindow w;
    w.showFullScreen();


    return a.exec();
}