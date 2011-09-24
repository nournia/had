#ifndef PLANVIEWER_H
#define PLANVIEWER_H

#include <QWidget>
#include <QMouseEvent>

#include <vector>
using namespace std;

class PlanViewer : public QWidget
{
    Q_OBJECT
public:
    explicit PlanViewer(QWidget *parent = 0, bool _thumbnail = false);

    void paintEvent(QPaintEvent * event);

    vector<double> genome;
    vector<QRectF> spaces;

    void setGenome(vector<double> g);

private:
    bool thumbnail;
    const int resizeWidth;
    int drag, resize_x1, resize_y1, resize_x2, resize_y2;

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    void paintOn(QPaintDevice * device, bool development, QSize page);

signals:
    void genomeChanged();
    void selected(vector<double> genome);

public slots:

};

#endif // PLANVIEWER_H
