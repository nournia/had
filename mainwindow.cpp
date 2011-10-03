#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDir>
#include <QDesktopWidget>
#include <QDateTime>

#include <evaluate.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->gGenome->setVisible(false);

    QFile file("had.param");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        ui->eCommand->setPlainText(file.readAll());

    connect(ui->viewer, SIGNAL(genomeChanged()), this, SLOT(displayEvaluations()));

    thread = new GAThread("");
    connect(thread, SIGNAL(finished()), this, SLOT(on_bExecute_clicked()));

    viewerThread = new ViewerThread(this);

    resize(800, 600);
    this->move(QApplication::desktop()->screen()->rect().center()-this->rect().center());

    connect(ui->cShow, SIGNAL(currentIndexChanged(int)), this, SLOT(on_bLoad_clicked()));
    connect(ui->sFeasible, SIGNAL(editingFinished()), this, SLOT(on_bLoad_clicked()));
    connect(ui->sSeed, SIGNAL(editingFinished()), this, SLOT(on_bExecute_clicked()));

    ui->grid->move(0, 0);
    ui->frame->hide();
}

void MainWindow::resizeEvent (QResizeEvent * event)
{
    const int panel = 200;
    ui->grid->resize(this->width() - panel, this->height());
    ui->panel->move(this->width() - panel, 0);
    ui->panel->resize(panel, this->height());

    ui->frame->resize(600, 400);
    ui->frame->move((this->width() - ui->frame->width())/2, (this->height() - ui->frame->height())/2);
}

void MainWindow::mouseMoveEvent (QMouseEvent * event)
{
    if (! ui->frame->underMouse())
    {
        ui->grid->setFocus();
        ui->frame->hide();
    }
}

void MainWindow::planClick(vector<double> genome)
{
    if (ui->frame->isVisible())
    {
        ui->grid->setFocus();
        ui->frame->hide();
        return;
    }

    showSolution(genome);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void ViewerThread::run()
{
    while(1)
    {
        msleep(200);
        ((MainWindow*)parent)->on_bLoad_clicked();
    }
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
    QString command = ui->eCommand->toPlainText().replace("\n", " ");
    command += " --seed=" + ui->sSeed->text();

    if (ui->bExecute->text() == "Execute")
    {
        ui->cShow->setCurrentIndex(0);

        thread->command = command;
        thread->start();
        viewerThread->start();

        ui->bExecute->setText("Stop");
        ui->bExecute->setEnabled(false);
    } else
    {
        thread->terminate();
        viewerThread->terminate();
        on_bLoad_clicked();

        ui->bExecute->setText("Execute");
        ui->bExecute->setEnabled(true);
    }

}

vector<double> getGenome(QString g)
{
    vector<double> genome;
    QStringList values = g.split(" ");
    int size = House::house->rooms * 4;

    int i = 0;
    for (; values[i].toDouble() != size; i++);
    for (int j = i+1; (j < values.size()) && (j - i <= size); j++)
        genome.push_back(values[j].toDouble());

    return genome;
}

double genomeDiff(vector<double> g1, vector<double> g2)
{
    double maxDiff = 0;
    for (size_t i = 0; i < g1.size(); i++)
        maxDiff = max(maxDiff, fabs(g1[i] - g2[i]));
    return maxDiff;
}

double genomeValue(QString g)
{
    return g.split(" ")[0].toDouble();
}

void MainWindow::addNewSelectedSolution(QString& item, QStringList& solutions)
{
    const double minDiff = 5;
    double maxPenalty = ui->sFeasible->value();

    vector<double> genome = getGenome(item);
    House::house->update(genome);

    if (House::house->getAreaPenalty() < maxPenalty && House::house->getIntersectionPenalty() < maxPenalty)
    {
        size_t newResult = true;
        for (size_t j = 0; j < solutions.size(); j++)
            if (genomeDiff(genome, getGenome(solutions[j])) < minDiff)
            {
                if (real_value(genome) < real_value(getGenome(solutions[j])))
                    solutions[j] = item;

                newResult = false;
                break;
            }

        if (newResult)
            solutions << item;
    }
}

void MainWindow::loadGeneration(int index)
{
    if (index < 0 || index > generations.size() - 1) return;
    gen = index;

    QFile file(generations[gen]);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    setWindowTitle(QString("Human Aided Design") + " - " + QFileInfo(generations[gen]).fileName());

    population.clear();

    while (!file.atEnd()) {
        QString line = file.readLine();

        if (line.startsWith("\\section{eoPop}"))
        {
           int size = file.readLine().trimmed().toInt();

           for (int i = 0; i < size; i++)
               population << file.readLine();
        }
    }

    // select solutions
    for (size_t i = 0; i < population.size(); i++)
        addNewSelectedSolution(population[i], selectedSolutions);

    // prune solutions
    if (ui->cShow->currentText() == tr("Feasibles"))
    {
        QStringList results;
        for (size_t i = 0; i < population.size(); i++)
            addNewSelectedSolution(population[i], results);
        population = results;
    }

    showPopulation();
}

void MainWindow::sortPopulation()
{
//    ui->lPopulation->setText(QString("%1").arg(population.size()));

    for (size_t j, i = 0; i < population.size(); i++)
        for (j = i+1; j < population.size(); j++)
            if (genomeValue(population[i]) > genomeValue(population[j]))
            {
                QString tmp = population[i];
                population[i] = population[j];
                population[j] = tmp;
            }
}

double present(double value)
{
    return round(1000 * value) / 1000;
}

void MainWindow::showSolution(vector<double> genome)
{
    ui->viewer->setGenome(genome);
    displayEvaluations();
    ui->frame->show();
}

void MainWindow::displayEvaluations()
{
    vector<double> genome = ui->viewer->genome;
    House* house = House::house;
    int size = house->rooms * 4;

    if (genome.size() < size)
        return;

    QString tmp = QString("%1").arg(size);
    for (int i = 0; i < size; i++)
        tmp += QString(" %1").arg(genome[i]);
    ui->eGenome->setText(tmp);

    ui->lSum->setText(QString("%1").arg(present(real_value(genome))));

    double areaPenalty = 0, intersectionPenalty = 0, accessPenalty = 0, lightPenalty = 0, spacePenalty = 0, sidePenalty = 0;
    areaPenalty = house->getAreaPenalty();
    intersectionPenalty = house->getIntersectionPenalty();
    sidePenalty = house->getSidePenalty();

    house->updateSpaces();
    vector<Rect>& spaces = house->spaces;
    ui->viewer->spaces.clear();
    for (int i = 0; i < spaces.size(); i++)
        ui->viewer->spaces.push_back(QRectF(spaces[i].x1, spaces[i].y1, spaces[i].x2 - spaces[i].x1, spaces[i].y2 - spaces[i].y1));
    ui->viewer->update();

    if (house->spaces.size() > 0)
    {
        spacePenalty = -1 * house->getSpaceProfit();
        lightPenalty = -1 *house->getLightProfit();
        accessPenalty = house->getAccessPenalty();
    }

    ui->lAreaPenalty->setText(QString("%1").arg(present(areaPenalty)));
    ui->lIntersectionPenalty->setText(QString("%1").arg(present(intersectionPenalty)));
    ui->lSidePenalty->setText(QString("%1").arg(present(sidePenalty)));
    ui->lAccessPenalty->setText(QString("%1").arg(present(accessPenalty)));
    ui->lLightPenalty->setText(QString("%1").arg(present(lightPenalty)));
    ui->lSpacePenalty->setText(QString("%1").arg(present(spacePenalty)));
}

void MainWindow::on_sGenerations_sliderMoved(int position)
{
    loadGeneration(position);
}

void MainWindow::on_bLoad_clicked()
{
    // load list of generation files
    generations.clear();
    QDir dir("/home/alireza/repo/had/input");
    QFileInfoList list = dir.entryInfoList();

    if (list.size() == 0) return;

    for (int i = 0; i < list.size(); i++)
        if (list.at(i).filePath().endsWith(".sav"))
            generations << list.at(i).filePath();

    FilenameLessThan le;
    qSort(generations.begin(), generations.end(), le);

    if (generations.count() > 0)
    {
        ui->sGenerations->setMaximum(generations.count()-1);
        ui->sGenerations->setValue(generations.count()-1);
        loadGeneration(generations.count()-1);
    }

    showPopulation();
}

void MainWindow::showPopulation()
{
    if (ui->cShow->currentText() == tr("Selected"))
        population = selectedSolutions;

    sortPopulation();

//    showSolution(getGenome(population[0]));

    // show population in grid
    if (plans.size() != population.count())
    {
        QLayoutItem *child;
        while ((child = ui->gridLayout->takeAt(0)) != 0)
        {
            delete child->widget();
            delete child;
        }

        plans.resize(population.count());

        // set number of columns
        int cols = 1;
        for (; cols*cols < population.size(); cols++);

        for (int i = 0; i < plans.size(); i++)
        {
            plans[i] = new PlanViewer(ui->grid, true);
            connect(plans[i], SIGNAL(selected(vector<double>)), this, SLOT(planClick(vector<double>)));
            ui->gridLayout->addWidget(plans[i], i / cols, i % cols);
        }
    }

    for (int i = 0; i < population.count(); i++)
        plans[i]->setGenome(getGenome(population[i]));
}

void MainWindow::on_bSaveImage_clicked()
{
    House* house = House::house;
    if (! house) return;

    QSize size(800, 600);

    double r = min(size.width() / house->original_width, size.height() / house->original_width);
    size.setWidth(round(r * house->original_width));
    size.setHeight(round(r * house->original_height));

    QImage* img = new QImage(size, QImage::Format_RGB32);
    ui->viewer->paintOn(img, false, size);

    QDir current;
    current.mkdir("img");
    img->save("img/" + QDateTime::currentDateTime().toString() + ".jpg", "jpg", 100);
}

void MainWindow::on_bGenome_clicked()
{
    ui->gGenome->setVisible(! ui->gGenome->isVisible());
}

void MainWindow::on_bApplyGenome_clicked()
{
    showSolution(getGenome(ui->eGenome->text()));
}
