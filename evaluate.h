
#ifndef EVALUATION_H
#define EVALUATION_H

#include <vector>
typedef const std::vector<double>& GENOME;

#include <math.h>
using namespace std;

namespace house {
    int rooms;
    double space_width, space_height;
    bool** accesses;
    double* areas;
}
using namespace house;

void initHouse()
{
    rooms = 8;
    space_width = 10;
    space_height = 10;

    // Areas
    areas = new double[8];
    // livingroom, kitchen, bedroom1, bedroom2, bathroom, toilet, stairs, elevator
    areas[0] = 35; areas[1] = 15; areas[2] = 9; areas[3] = 12; areas[4] = 3; areas[5] = 4; areas[6] = 10.5; areas[7] = 2.5;

    // Accesses
    accesses = new bool*[rooms];
    for (int i = 0; i < rooms; i++)
    {
        accesses[i] = new bool[rooms];
        for (int j = 0; j < rooms; j++)
             accesses[i][j] = 0;
    }

     accesses[0][1] = 1; accesses[0][2] = 1; accesses[0][3] = 1; accesses[0][5] = 1; accesses[0][6] = 1; // "livingroom": ["kitchen", "bedroom1", "bedroom2", "toilet", "stairs"]
     accesses[3][4] = 1; // "bedroom2": ["bathroom"]
     accesses[6][7] = 1; // "stairs": ["elevator"]
}


inline double getWidth(GENOME genome, int i) { return genome[i*4+2]; }
inline double getHeight(GENOME genome, int i) { return genome[i*4+3]; }
inline double getArea(GENOME genome, int i) { return genome[i*4+2] * genome[i*4+3]; }
inline bool isEqual(double a, double b) { return fabs(a-b) < 0.001; }

// Geometry

class Rect {

public:
    double x1, y1, x2, y2;

    Rect(double _x1, double _y1, double _x2, double _y2)
        :x1(_x1), y1(_y1), x2(_x2), y2(_y2)
    {}

    Rect(GENOME genome, int index)
    {
        index *= 4;
        x1 = genome[index];
        y1 = genome[index+1];
        x2 = genome[index] + genome[index+2];
        y2 = genome[index+1] + genome[index+3];
    }
};


inline double getIntersectionArea(const Rect& r1, const Rect& r2)
{
    double area = (max(r1.x1, r2.x1) - min(r1.x2, r2.x2)) * (max(r1.y1, r2.y1) - min(r1.y2, r2.y2));
    return area > 0 ? area : 0;
}

// positve means rectangls don't have intersection
double getEdgeDistance(const Rect& r1, const Rect& r2)
{
    double l = r1.x1 - r2.x2, r = r1.x2 - r2.x1,
           u = r1.y1 - r2.y2, d = r1.y2 - r2.y1;

    double minX = min(fabs(l), fabs(r)) * (l*r < 0 ? -1 : 1),
           minY = min(fabs(u), fabs(d)) * (u*d < 0 ? -1 : 1);

    return fabs(minX) < fabs(minY) ? minX : minY;
}


// Penalty functions

double getAreaPenalty(GENOME genome, int index)
{
    return fabs(areas[index] - getArea(genome, index));
}

double getProportionPenalty(GENOME genome, int index)
{
    double ratio = getWidth(genome, index) / getHeight(genome, index);
    if (ratio < 0)
        return 0; // invalid size - no penalty

    if (ratio > 1) ratio = 1 / ratio;

    if (ratio < 1 && ratio > 0.7)
        return 0; // good ratio

    return pow(2, 1/ratio);
}

double getSpaceBoundaryPenalty(GENOME genome, int index)
{
    static Rect r1(0, 0, space_width, space_height);
    Rect r2(genome, index);

    double l = r1.x1 - r2.x1, r = r2.x2 - r1.x2,
           u = r1.y1 - r2.y1, d = r2.y2 - r1.y2;

    return (l > 0 ? l : 0) + (r > 0 ? r : 0) + (u > 0 ? u : 0) + (d > 0 ? d : 0);
}

double getIntersectionPenalty(GENOME genome, int first, int second)
{
    const Rect &r1 = Rect(genome, first), &r2 = Rect(genome, second);

    double distance = getEdgeDistance(r1, r2);

    return distance < 0 ? fabs(distance) : 0;
}

const int offset = 1; // door width
inline double getAlign(double r11, double r12, double r21, double r22)
{
    double align = 0;
    double a1 = r11 + offset, a2 = r12 - offset;
    if (r22 < a1) align = a1 - r22;
    if (r21 > a2) align = r21 - a2;
    if (r22 < a1 && r21 > a2) align = 0;
    return align;
}


double getAccessPenalty(GENOME genome, int first, int second)
{
    if (accesses[first][second])
    {
        const Rect &r1 = Rect(genome, first), &r2 = Rect(genome, second);

        double l = r1.x1 - r2.x2, r = r1.x2 - r2.x1,
               u = r1.y1 - r2.y2, d = r1.y2 - r2.y1;

        double minX = min(fabs(l), fabs(r)),
               minY = min(fabs(u), fabs(d));

        double xalign = getAlign(r1.x1, r1.x2, r2.x1, r2.x2),
               yalign = getAlign(r1.y1, r1.y2, r2.y1, r2.y2);

        return min(minX + xalign, minY + yalign);
    }

    return 0;
}

// Evaluate

double real_value(GENOME genome)
{
    double penalty = 0;

    if (!rooms)
        initHouse();

    for (int j, i = 0; i < rooms; i++)
     {
         penalty += getAreaPenalty(genome, i);
         penalty += getProportionPenalty(genome, i);
         penalty += getSpaceBoundaryPenalty(genome, i);

         for (j = i+1; j < rooms; j++)
         {
             penalty += getIntersectionPenalty(genome, i, j);
             penalty += getAccessPenalty(genome, i, j);
         }
     }

    return penalty;
}

#endif
