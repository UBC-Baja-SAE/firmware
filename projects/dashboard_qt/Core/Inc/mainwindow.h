#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

// Forward declaration for the auto-generated UI class
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateDisplay(); // Called by timer to update UI with latest data

private:
    Ui::MainWindow *ui; // Pointer to the graphical elements
    QTimer *updateTimer; // Timer for periodic UI updates
};

#endif // MAINWINDOW_H