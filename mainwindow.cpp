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

    resize(800, 600);
    this->move(QApplication::desktop()->screen()->rect().center()-this->rect().center());
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
    QString command = ui->eCommand->toPlainText().replace("\n", " ");
    command += " --seed=" + ui->sSeed->text();

    if (ui->bExecute->text() == "Execute")
    {
        thread->command = command;
        thread->start();
        ui->bExecute->setText("Stop");
        ui->bExecute->setEnabled(false);
    } else
    {
        thread->quit();
        ui->bExecute->setText("Execute");
        ui->bExecute->setEnabled(true);
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

void MainWindow::loadPopulation(int index, QString answer)
{
    QStringList values;

    if (!answer.isEmpty())
       values = answer.split(" ");
    else
    {
        if (index < 0) index = 0;
        if (index >= populations.count()) index = populations.count() - 1;
        pop = index;

        values = populations[pop].split(" ");
    }

    int size = House::house->rooms * 4;

    vector<double> genome;
    int i = 0;
    for (; values[i].toDouble() != size; i++);
    for (int j = i+1; (j < values.size()) && (j - i <= size); j++)
        genome.push_back(values[j].toDouble());

    ui->viewer->setGenome(genome);
    displayEvaluations();
}

void MainWindow::displayEvaluations()
{
    vector<double> genome = ui->viewer->genome;

    int size = House::house->rooms * 4;
    QString tmp = QString("%1").arg(size);
    for (int i = 0; i < size; i++)
        tmp += QString(" %1").arg(genome[i]);
    ui->eGenome->setText(tmp);

    ui->lSum->setText(QString("%1").arg(present(real_value(genome))));

    House* house = House::house;

    double areaPenalty = 0, intersectionPenalty = 0, accessPenalty = 0, lightPenalty = 0, spacePenalty = 0, sidePenalty = 0;
    areaPenalty = house->getAreaPenalty();
    sidePenalty = house->getSidePenalty();

    intersectionPenalty = house->getIntersectionPenalty();

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
    loadGeneration(position);
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

    if (generations.count() > 0)
    {
        ui->sGenerations->setMaximum(generations.count()-1);
        ui->sGenerations->setValue(generations.count()-1);
        loadGeneration(generations.count()-1);
    }
}

void MainWindow::on_sSeed_editingFinished()
{
    ui->bExecute->click();
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
    loadPopulation(-1, ui->eGenome->text());
}
