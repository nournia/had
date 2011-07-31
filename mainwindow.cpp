#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

#include <evaluate.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_bExecute_clicked()
{
    QString command = "/home/alireza/repo/EO-1.2.0/eo/release/tutorial/Lesson4/had";

    command += " --seed=" + ui->sSeed->text();

//    ######    ES mutation             ######
//    # --Isotropic=1                            # -i : Isotropic self-adaptive mutation
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
    command += " --sigmaInit=0.3%";

//    ######    Output                  ######
//    # --useEval=1                              # Use nb of eval. as counter (vs nb of gen.)
//    # --useTime=1                              # Display time (s) every generation
//    command += " --printBestStat=1";
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
    command += " --saveFrequency=50";
//    # --saveTimeInterval=0                     # Save every T seconds (0 or absent = never)
//    # --status=t-eoESAll.status                # Status file

//    ######    Stopping criterion      ######
    command += " --maxGen=10000";
    command += " --steadyGen=100";
//    # --minGen=0                               # -g : Minimum number of generations
//    # --maxEval=0                              # -E : Maximum number of evaluations (0 = none)
//    # --targetFitness=0                        # -T : Stop when fitness reaches
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

    system(command.toAscii());
}
