#include "planviewer.h"

#include <QPainter>

PlanViewer::PlanViewer(QWidget *parent) :
    QWidget(parent)
{
}

void PlanViewer::setGenome(vector<double> g)
{
    genome = g;
    update();
}

void PlanViewer::paintEvent(QPaintEvent * event)
{
    if (genome.size() <= 0)
        return;

    QStringList rooms = QStringList() << "livingroom" << "kitchen" << "bedroom1" << "bedroom2" << "bathroom" << "toilet" << "stairs" << "elevator";

    double space_width = 10, space_height = 10;
    double r = min(width() / space_width, height() / space_height);

    QPainter painter(this);
    for (int i = 0; i < rooms.count(); i++)
    {
        QRect room(r * genome[4*i], r * genome[4*i+1], r * genome[4*i+2], r * genome[4*i+3]);
        painter.drawRect(room);
        painter.drawText(room, Qt::AlignCenter, rooms[i]);
    }

    painter.drawRect(QRect(0, 0, r * space_width, r * space_height));
}
