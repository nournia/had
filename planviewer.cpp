#include "planviewer.h"

#include <QPainter>
#include <QColor>
#include <QDebug>

#include <math.h>

PlanViewer::PlanViewer(QWidget *parent) :
    QWidget(parent), resizeWidth(5)
{
    mouseReleaseEvent(0);
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
    paintOn(this, true, size());
}

void PlanViewer::paintOn(QPaintDevice * device, bool development, QSize page)
{
    if (genome.size() <= 0)
        return;

    QStringList rooms = QStringList() << "kitchen" << "bedroom1" << "bedroom2" << "bathroom" << "toilet" << "stairs" << "elevator";

    space_width = 10.6, space_height = 10.05, wall = 0.15, out_wall = 0.3;
    r = min(page.width() / space_width, page.height() / space_height);

    QPainter painter(device);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(255, 255, 255)));
    painter.drawRect(QRect(0, 0, round(r * space_width), round(r * space_height)));

    for (int i = 0; i < rooms.count(); i++)
    {
        QRect room(round(r * (genome[4*i] + out_wall)), round(r * (genome[4*i+1] + out_wall)), round(r * (genome[4*i+2] - wall)), round(r * (genome[4*i+3] - wall)));

        painter.setPen(Qt::SolidLine);
        painter.drawText(room, Qt::AlignCenter, rooms[i]);

        painter.setPen(QColor(50, 50, 50, 120));
        painter.setBrush(QBrush(QColor(0, 0, 255, 30)));
        painter.drawRect(room);

//        // resize button
//        if (development)
//        {
//            painter.setPen(Qt::NoPen);
//            painter.setBrush(QBrush(QColor(0, 255, 0, 100)));
//            double x1 = room.bottomRight().x() - resizeButtonWidth, y1 = room.bottomRight().y() - resizeButtonWidth;
//            painter.drawRect(x1, y1, resizeButtonWidth, resizeButtonWidth);
//        }
    }


    // Spaces
    if (development)
    {
        painter.setPen(QColor(50, 50, 50, 120));

        for (int i = 0; i < spaces.size(); i++)
        {
            if (i == 0)
                painter.setBrush(QBrush(QColor(255, 255, 0, 20)));
            else
                painter.setBrush(QBrush(QColor(50, 50, 50, 20)));

            QRect space(r * (spaces[i].left() + out_wall), r * (spaces[i].top() + out_wall), r * (spaces[i].width() - wall), r * (spaces[i].height() - wall));
            painter.drawRect(space);
        }
    }
}

QPoint lastPos;
void PlanViewer::mousePressEvent(QMouseEvent *event)
{
    if (genome.size() == 0)
        return;

    for (int i = 0; i < 7; i++)
    {
        QRect room(round(r * (genome[4*i] + out_wall)), round(r * (genome[4*i+1] + out_wall)), round(r * (genome[4*i+2] - wall)), round(r * (genome[4*i+3] - wall)));

        if (event->pos().x() >= room.x() && event->pos().y() >= room.y() && event->pos().x() <= room.x() + room.width() && event->pos().y() <= room.y() + room.height())
        {
            if (event->pos().x() >= room.x() + room.width() - resizeWidth)
                resize_x2 = i;
            else if (event->pos().x() <= room.x() + resizeWidth)
                resize_x1 = i;

            if (event->pos().y() >= room.y() + room.height() - resizeWidth)
                resize_y2 = i;
            else if (event->pos().y() <= room.y() + resizeWidth)
                resize_y1 = i;

            if (resize_x1 == -1 && resize_y1 == -1 && resize_x2 == -1 && resize_y2 == -1)
                drag = i;

            break;
        }
    }

    lastPos.setX(event->pos().x()); lastPos.setY(event->pos().y());
}

void PlanViewer::mouseMoveEvent(QMouseEvent *event)
{
    bool change = false;
    double xdiff = (event->pos().x() - lastPos.x())/r, ydiff = (event->pos().y() - lastPos.y())/r;

    if (drag >= 0)
    {
        genome[4*drag] += xdiff;
        genome[4*drag+1] += ydiff;
        change = true;
    }

    if (resize_x1 >= 0)
    {
        genome[4*resize_x1] += xdiff;
        genome[4*resize_x1+2] -= xdiff;
        change = true;
    }
    else if (resize_x2 >= 0)
    {
        genome[4*resize_x2+2] += xdiff;
        change = true;
    }

    if (resize_y1 >= 0)
    {
        genome[4*resize_y1+1] += ydiff;
        genome[4*resize_y1+3] -= ydiff;
        change = true;
    }
    else if (resize_y2 >= 0)
    {
        genome[4*resize_y2+3] += ydiff;
        change = true;
    }

    if (change)
    {
        update();
        emit genomeChanged();
    }

    lastPos.setX(event->pos().x()); lastPos.setY(event->pos().y());
}

void PlanViewer::mouseReleaseEvent(QMouseEvent *event)
{
    drag = -1;
    resize_x1 = -1;
    resize_y1 = -1;
    resize_x2 = -1;
    resize_y2 = -1;
}
