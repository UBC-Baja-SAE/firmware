#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QByteArray>
#include <QPixmap>
#include <atomic>


// Forward declaration for the auto-generated UI class
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    signals:
        void newFrameReceived(const QByteArray& frameData);

private slots:
    void updateDisplay(); // Called by timer to update UI with latest data
    void updateCameraDisplay(const QByteArray& frameData);

private:
    Ui::MainWindow *ui; // Pointer to the graphical elements
    QTimer *updateTimer; // Timer for periodic UI updates
    std::atomic<bool> is_ui_busy{false}; // Add this!
};

#endif // MAINWINDOW_H