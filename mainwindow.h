#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QThread>
#include <QProcess>
#include <QMainWindow>

class GAThread : public QThread
{
public:
    QString command;

    GAThread(QString cmd)
        : command(cmd)
    {}

    void run()
    {
        static QString lastCommand;

        if (command != lastCommand)
        {
            lastCommand = command;
            QProcess process;
            process.execute(command);
        }
    }
};


class ViewerThread : public QThread
{
public:
    QWidget* parent;

    ViewerThread(QWidget* _parent)
        : parent(_parent)
    {}

    void run();
};


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
    ViewerThread* viewerThread;

    void loadGeneration(int index);
    void loadPopulation(int index, QString answer = "");

    void pruneSolutions(); // a diverse set of feasible solutions

public slots:
    void displayEvaluations();

    void on_bExecute_clicked();

    void on_bNext_clicked();

    void on_bPrevious_clicked();

    void on_sGenerations_sliderMoved(int position);

    void on_bLoad_clicked();

    void on_sSeed_editingFinished();

    void on_bSaveImage_clicked();

    void on_bGenome_clicked();

    void on_bApplyGenome_clicked();

    void on_chFeasible_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
