
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

const int distanceCoeff = 1, areaCoeff = 1;

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
inline bool isEqual(double a, double b) { return abs(a-b) < 0.001; }


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

double getEdgeDistance(const Rect& r1, const Rect& r2)
{
    return min(min(min(abs(r1.x1 - r2.x1), abs(r1.x1 - r2.x2)), min(abs(r1.x2 - r2.x1), abs(r1.x2 - r2.x2))),
               min(min(abs(r1.y1 - r2.y1), abs(r1.y1 - r2.y2)), min(abs(r1.y2 - r2.y1), abs(r1.y2 - r2.y2))));

}


// Penalty functions

double getAreaPenalty(GENOME genome, int index)
{
    return areaCoeff * abs(areas[index] - getArea(genome, index));
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
    static Rect space(0, 0, space_width, space_height);
    return areaCoeff * (getArea(genome, index) - getIntersectionArea(Rect(genome, index), space));
}

double getIntersectionPenalty(GENOME genome, int first, int second)
{
    const Rect &r1 = Rect(genome, first), &r2 = Rect(genome, second);

    double intersection = getIntersectionArea(r1, r2);
    double distancePenalty = 0;

    if (isEqual(intersection, getArea(genome, first)) || isEqual(intersection, getArea(genome, second)))
        distancePenalty = getEdgeDistance(r1, r2);

    return areaCoeff * intersection + distanceCoeff * distancePenalty;
}

double getAccessPenalty(GENOME genome, int first, int second)
{
    if (accesses[first][second])
        return distanceCoeff * getEdgeDistance(Rect(genome, first), Rect(genome, second));

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
