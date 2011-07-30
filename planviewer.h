#ifndef PLANVIEWER_H
#define PLANVIEWER_H

#include <QWidget>

#include <vector>
using namespace std;

class PlanViewer : public QWidget
{
    Q_OBJECT
public:
    explicit PlanViewer(QWidget *parent = 0);

    void paintEvent(QPaintEvent * event);

    vector<double> genome;

    void setGenome(vector<double> g);
signals:

public slots:

};

#endif // PLANVIEWER_H
