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
    explicit PlanViewer(QWidget *parent = 0);

    void paintEvent(QPaintEvent * event);

    vector<double> genome;
    vector<QRectF> spaces;

    void setGenome(vector<double> g);

private:
    const int resizeButtonWidth;
    int drag, resize;

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    void paintOn(QPaintDevice * device, bool development, QSize page);

signals:
    void genomeChanged();

public slots:

};

#endif // PLANVIEWER_H
