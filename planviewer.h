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
    int drag;
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

signals:
    void genomeChanged();

public slots:

};

#endif // PLANVIEWER_H
