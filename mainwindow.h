#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QStringList generations, populations;
    int gen, pop;

    void loadGeneration(int index);
    void loadPopulation(int index);


private slots:
    void on_bExecute_clicked();

    void on_bNext_clicked();

    void on_bPrevious_clicked();

    void on_sGenerations_sliderMoved(int position);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
