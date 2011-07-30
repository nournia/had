
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


// Geometry functions

inline double getEdgeDistance(double r1x1, double r1y1, double r1x2, double r1y2, double r2x1, double r2y1, double r2x2, double r2y2)
{
    return min(min(min(abs(r1x1 - r2x1), abs(r1x1 - r2x2)), min(abs(r1x2 - r2x1), abs(r1x2 - r2x2))),
               min(min(abs(r1y1 - r2y1), abs(r1y1 - r2y2)), min(abs(r1y2 - r2y1), abs(r1y2 - r2y2))));
}

inline double getIntersectionArea(double r1x1, double r1y1, double r1x2, double r1y2, double r2x1, double r2y1, double r2x2, double r2y2)
{
    double area = (max(r1x1, r2x1) - min(r1x2, r2x2)) * (max(r1y1, r2y1) - min(r1y2, r2y2));
    return area > 0 ? area : 0;
}

double getEdgeDistance(GENOME genome, int first, int second)
{
    return getEdgeDistance(genome[first], genome[first+1], genome[first] + genome[first+2], genome[first+1] + genome[first+3],
                           genome[second], genome[second+1], genome[second] + genome[second+2], genome[second+1] + genome[second+3]);
}

double getIntersectionArea(GENOME genome, int first, int second)
{
    return getEdgeDistance(genome[first], genome[first+1], genome[first] + genome[first+2], genome[first+1] + genome[first+3],
                           genome[second], genome[second+1], genome[second] + genome[second+2], genome[second+1] + genome[second+3]);
}

double getIntersectionAreaWithSpace(GENOME genome, int index)
{
    return getEdgeDistance(genome[index], genome[index+1], genome[index] + genome[index+2], genome[index+1] + genome[index+3],
                           0, 0, space_width, space_height);
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
    return areaCoeff * (getArea(genome, index) - getIntersectionAreaWithSpace(genome, index));
}

double getIntersectionPenalty(GENOME genome, int first, int second)
{
    double intersection = getIntersectionArea(genome, first, second);
    double distancePenalty = 0;

    if (isEqual(intersection, getArea(genome, first)) || isEqual(intersection, getArea(genome, second)))
        distancePenalty = getEdgeDistance(genome, first, second);

    return areaCoeff * intersection + distanceCoeff * distancePenalty;
}

double getAccessPenalty(GENOME genome, int first, int second)
{
    if (accesses[first][second])
        return distanceCoeff * getEdgeDistance(genome, first, second);

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
