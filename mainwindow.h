#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QThread>
#include <QProcess>

class GAThread : public QThread
 {
 public:
    QString command;

    GAThread(QString cmd)
        : command(cmd)
    {}

    void run()
    {
        QProcess process;
        process.execute(command);
    }
 };


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

    GAThread* thread;

    void loadGeneration(int index);
    void loadPopulation(int index, QString answer = "");


private slots:
    void displayEvaluations();

    void on_bExecute_clicked();

    void on_bNext_clicked();

    void on_bPrevious_clicked();

    void on_sGenerations_sliderMoved(int position);

    void on_bLoad_clicked();

    void on_bSample_clicked();

    void on_sSeed_editingFinished();

    void on_bSaveImage_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
