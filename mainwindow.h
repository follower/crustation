#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(crustFileLoad)

namespace Ui {
class MainWindow;
}

class GpuCommand;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void command_onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

    void loadFile(QString logFilePath);

private slots:
    void on_actionOpen_triggered();

    void on_actionPlay_triggered();

    void on_actionPause_triggered();

    void on_selectPlayback1second_triggered();

    void on_selectPlayback200ms_triggered();

    void on_selectPlayback10ms_triggered();

private:
    Ui::MainWindow *ui;
    void drawPolygon(QPainter &painter, GpuCommand *current_command, bool useItemColor);
    void initVram();
    void setupMoreUi();

    bool isPlaying;
    void playNextCommand();
};

#endif // MAINWINDOW_H
