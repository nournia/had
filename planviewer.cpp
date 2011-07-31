#include "planviewer.h"

#include <QPainter>
#include <QColor>

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

    int page_width = width() - 2, page_height = height() - 2;

    QStringList rooms = QStringList() << "livingroom" << "kitchen" << "bedroom1" << "bedroom2" << "bathroom" << "toilet" << "stairs" << "elevator";

    double space_width = 10, space_height = 10;
    double r = min(page_width / space_width, page_height / space_height);

    QPainter painter(this);
    painter.setBrush(QBrush(QColor("white")));
    painter.drawRect(QRect(0, 0, r * space_width, r * space_height));

    for (int i = 0; i < rooms.count(); i++)
    {
        QRect room(r * genome[4*i], r * genome[4*i+1], r * genome[4*i+2], r * genome[4*i+3]);
        painter.drawRect(room);
        painter.drawText(room, Qt::AlignCenter, rooms[i]);
    }
}
