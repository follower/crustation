#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private:
    Ui::MainWindow *ui;
    void drawPolygon(QPainter &painter, GpuCommand *current_command, bool useItemColor);
    void initUi();
};

#endif // MAINWINDOW_H
