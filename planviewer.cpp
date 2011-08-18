#include "planviewer.h"

#include <QPainter>
#include <QColor>
#include <QDebug>

PlanViewer::PlanViewer(QWidget *parent) :
    QWidget(parent), resizeButtonWidth(5)
{
    drag = -1;
    resize = -1;
}

void PlanViewer::setGenome(vector<double> g)
{
    genome = g;
    update();
}

double space_width, space_height, wall, out_wall;
double r;

void PlanViewer::paintEvent(QPaintEvent * event)
{
    if (genome.size() <= 0)
        return;

    int page_width = width() - 2, page_height = height() - 2;

    QStringList rooms = QStringList() << "kitchen" << "bedroom1" << "bedroom2" << "bathroom" << "toilet" << "stairs" << "elevator";

    space_width = 10.6, space_height = 10.05, wall = 0.15, out_wall = 0.3;
    r = min(page_width / space_width, page_height / space_height);


    QPainter painter(this);
    painter.setBrush(QBrush(QColor(255, 255, 255)));
//    painter.setBrush(QBrush(QColor("white")));
    painter.drawRect(QRect(0, 0, r * space_width, r * space_height));

    for (int i = 0; i < rooms.count(); i++)
    {
        QRect room(r * (genome[4*i] + out_wall), r * (genome[4*i+1] + out_wall), r * (genome[4*i+2] - wall), r * (genome[4*i+3] - wall));

        painter.setPen(Qt::SolidPattern);
        painter.drawText(room, Qt::AlignCenter, rooms[i]);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(QColor(0, 0, 255, 20)));
        painter.drawRect(room);

        // resize button
        painter.setBrush(QBrush(QColor(0, 255, 0, 100)));
        double x1 = room.bottomRight().x() - resizeButtonWidth, y1 = room.bottomRight().y() - resizeButtonWidth;
        painter.drawRect(x1, y1, resizeButtonWidth, resizeButtonWidth);
    }


    // Spaces
    painter.setPen(QColor(50, 50, 50, 120));
    painter.setBrush(QBrush(QColor("blue"), Qt::BDiagPattern));
    for (int i = 0; i < spaces.size(); i++)
    {
        QRect space(r * (spaces[i].left() + out_wall), r * (spaces[i].top() + out_wall), r * (spaces[i].width() - wall), r * (spaces[i].height() - wall));
        painter.drawRect(space);
    }
}

QPoint lastPos;
void PlanViewer::mousePressEvent(QMouseEvent *event)
{
    if (genome.size() == 0)
        return;

    for (int i = 0; i < 7; i++)
    {
        QRect room(r * (genome[4*i] + out_wall), r * (genome[4*i+1] + out_wall), r * (genome[4*i+2] - wall), r * (genome[4*i+3] - wall));

        if (event->pos().x() >= room.x() && event->pos().y() >= room.y() && event->pos().x() <= room.x() + room.width() && event->pos().y() <= room.y() + room.height())
        {
            if (event->pos().x() >= room.x() + room.width() - resizeButtonWidth && event->pos().y() >= room.y() + room.height() - resizeButtonWidth)
                resize = i;
            else
                drag = i;

            break;
        }
    }

    lastPos.setX(event->pos().x()); lastPos.setY(event->pos().y());
}

void PlanViewer::mouseMoveEvent(QMouseEvent *event)
{
    if (drag >= 0)
    {
        genome[4*drag] += (event->pos().x() - lastPos.x())/r;
        genome[4*drag+1] += (event->pos().y() - lastPos.y())/r;

        update();
        emit genomeChanged();
    }

    if (resize >= 0)
    {
        genome[4*resize+2] += (event->pos().x() - lastPos.x())/r;
        genome[4*resize+3] += (event->pos().y() - lastPos.y())/r;

        update();
        emit genomeChanged();
    }

    lastPos.setX(event->pos().x()); lastPos.setY(event->pos().y());
}

void PlanViewer::mouseReleaseEvent(QMouseEvent *event)
{
    drag = -1;
    resize = -1;
}
