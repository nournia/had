#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QThread>
#include <QProcess>
#include <QMainWindow>

#include <planviewer.h>

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

    QStringList generations, population, selectedSolutions, processedFiles;
    int gen;

    GAThread* thread;
    ViewerThread* viewerThread;

    vector<PlanViewer*> plans;

    void loadGeneration(int index);
    void sortPopulation();
    void addNewSelectedSolution(QString& item, QStringList& solutions);

    void showSolution(vector<double> genome);
    void showPopulation();

    void resizeEvent (QResizeEvent * event);
    void mouseMoveEvent (QMouseEvent * event);

public slots:
    void displayEvaluations();

    void on_bExecute_clicked();

    void on_sGenerations_sliderMoved(int position);

    void on_bLoad_clicked();

    void on_bSaveImage_clicked();

    void on_bGenome_clicked();

    void on_bApplyGenome_clicked();

    void planClick(vector<double> genome);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
