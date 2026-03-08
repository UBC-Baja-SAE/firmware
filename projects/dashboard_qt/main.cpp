#include "mainwindow.h"
#include <QApplication>

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    MainWindow w;

    // w.show() opens it in a normal window.
    // w.showFullScreen() is usually what you want for a dedicated Pi dashboard!
    w.show();

    return a.exec();
}