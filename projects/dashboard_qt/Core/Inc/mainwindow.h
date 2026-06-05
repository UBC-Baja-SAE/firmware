#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QByteArray>
#include <QPixmap>
#include <QStackedWidget>
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
    void updateDisplay();
    void updateCameraDisplay(const QByteArray& frameData);

private:
    Ui::MainWindow *ui;
    QTimer *updateTimer;
    std::atomic<bool> is_ui_busy{false};

    void slideTo(int newIndex, bool slideLeft = true);
    bool m_animating = false;
    int currentIndex = 0;
};

#endif // MAINWINDOW_H