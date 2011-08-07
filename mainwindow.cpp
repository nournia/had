#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDir>

#include <evaluate.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    thread = new GAThread("");
    connect(thread, SIGNAL(finished()), this, SLOT(on_bExecute_clicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

// filename: "generations#.sav"
struct FilenameLessThan {
    bool operator()(const QString &s1, const QString &s2) const {

        int start = s1.indexOf("generations") + QString("generations").size();
        int i1 =  s1.mid(start, s1.size() - start - 4).toInt();
        int i2 =  s2.mid(start, s2.size() - start - 4).toInt();
        return i1 < i2;
    }
};


void MainWindow::on_bExecute_clicked()
{
    QString command = "/home/alireza/repo/EO-1.2.0/eo/release/tutorial/Lesson4/had";

    command += " --seed=" + ui->sSeed->text();

//    ######    ES mutation             ######
    command += " --Isotropic=1";
//    # --Stdev=0                                # -s : One self-adaptive stDev per variable
//    # --Correl=0                               # -c : Use correlated mutations

//    ######    Evolution Engine        ######
    command += " --popSize=20";
    command += " --selection=Sequential";
    command += " --nbOffspring=700%";
    command += " --replacement=Comma";
    command += " --weakElitism=0";

//    ######    Genotype Initialization    ######
    command += " --vecSize=32";
    command += " --initBounds=32[0,6]";
    command += " --sigmaInit=0.8%";

//    ######    Output                  ######
//    # --useEval=1                              # Use nb of eval. as counter (vs nb of gen.)
//    # --useTime=1                              # Display time (s) every generation
    command += " --printBestStat=0";
//    # --printPop=0                             # Print sorted pop. every gen.

//    ######    Output - Disk           ######
    command += " --resDir=/home/alireza/repo/had/input";
    command += " --eraseDir=1";
//    # --fileBestStat=1                         # Output bes/avg/std to file

//    ######    Output - Graphical      ######
//    # --plotBestStat=0                         # Plot Best/avg Stat
//    # --plotHisto=0                            # Plot histogram of fitnesses

//    ######    Persistence             ######
//    # --Load=                                  # -L : A save file to restart from
//    # --recomputeFitness=0                     # -r : Recompute the fitness after re-loading the pop.?
    command += " --saveFrequency=20";
//    # --saveTimeInterval=0                     # Save every T seconds (0 or absent = never)
//    # --status=t-eoESAll.status                # Status file

//    ######    Stopping criterion      ######
    command += " --maxGen=10000";
    command += " --steadyGen=100";
//    # --minGen=0                               # -g : Minimum number of generations
//    # --maxEval=0                              # -E : Maximum number of evaluations (0 = none)
    command += " --targetFitness=0";
//    # --CtrlC=0                                # -C : Terminate current generation upon Ctrl C

//    ######    Variation Operators     ######
    command += " --objectBounds=32[0,10]";
//    # --operator=SGA                           # -o : Description of the operator (SGA only now)
//    # --pCross=1                               # -C : Probability of Crossover
//    # --pMut=1                                 # -M : Probability of Mutation
//    # --crossType=global                       # -C : Type of ES recombination (global or standard)
//    # --crossObj=discrete                      # -O : Recombination of object variables (discrete, intermediate or none)
//    # --crossStdev=intermediate                # -S : Recombination of mutation strategy parameters (intermediate, discrete or none)
//    # --TauLoc=1                               # -l : Local Tau (before normalization)

    if (ui->bExecute->text() == "Execute")
    {
        thread->command = command;
        thread->start();
        ui->bExecute->setText("Stop");
    } else
    {
        thread->quit();
        ui->bExecute->setText("Execute");
        on_bLoad_clicked();
    }

}

void MainWindow::loadGeneration(int index)
{
    gen = index;

    QFile file(generations[gen]);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    populations.clear();
    while (!file.atEnd()) {
        QString line = file.readLine();

        if (line.startsWith("\\section{eoPop}"))
        {
           int size = file.readLine().trimmed().toInt();

           for (int i = 0; i < size; i++)
               populations << file.readLine();
        }
    }

    loadPopulation(0);
}

double present(double value)
{
    return round(1000 * value) / 1000;
}

void MainWindow::loadPopulation(int index)
{
    if (index < 0) index = 0;
    if (index >= populations.count()) index = populations.count() - 1;
    pop = index;

    QStringList values = populations[pop].split(" ");

    int size = values[2].toInt();

    vector<double> genome;
    for (int i = 3; i < size+3; i++)
        genome.push_back(values[i].toDouble());

    ui->viewer->setGenome(genome);
    ui->lSum->setText(QString("%1").arg(present(real_value(genome))));

    double areaPenalty = 0, proportionPenalty = 0, boundaryPenalty = 0, intersectionPenalty = 0, accessPenalty = 0, lightPenalty = 0;
    for (int j, i = 0; i < rooms; i++)
     {
         areaPenalty += getAreaPenalty(genome, i);
         proportionPenalty += getProportionPenalty(genome, i);
         boundaryPenalty += getBoundaryPenalty(genome, i);
         lightPenalty += getLightPenalty(genome, i);

         for (j = i+1; j < rooms; j++)
         {
             intersectionPenalty += getIntersectionPenalty(genome, i, j);
             accessPenalty += getAccessPenalty(genome, i, j);
         }
     }

    ui->lAreaPenalty->setText(QString("%1").arg(present(areaPenalty)));
    ui->lProportionPenalty->setText(QString("%1").arg(present(proportionPenalty)));
    ui->lBoundaryPenalty->setText(QString("%1").arg(present(boundaryPenalty)));
    ui->lIntersectionPenalty->setText(QString("%1").arg(present(intersectionPenalty)));
    ui->lAccessPenalty->setText(QString("%1").arg(present(accessPenalty)));
    ui->lLightPenalty->setText(QString("%1").arg(present(lightPenalty)));
}

void MainWindow::on_bNext_clicked()
{
    loadPopulation(pop + 1);
}

void MainWindow::on_bPrevious_clicked()
{
    loadPopulation(pop - 1);
}

void MainWindow::on_sGenerations_sliderMoved(int position)
{
    loadGeneration(ui->sGenerations->value());
}

void MainWindow::on_bLoad_clicked()
{
    // load input
    generations.clear();
    QDir dir("/home/alireza/repo/had/input");
    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); i++)
        if (list.at(i).filePath().endsWith(".sav"))
            generations << list.at(i).filePath();

    FilenameLessThan le;
    qSort(generations.begin(), generations.end(), le);

    ui->sGenerations->setMaximum(generations.count()-1);
    ui->sGenerations->setValue(generations.count()-1);
    loadGeneration(generations.count()-1);
}
