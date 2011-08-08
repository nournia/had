
#ifndef EVALUATION_H
#define EVALUATION_H

#include <algorithm>
#include <vector>
#include <math.h>
using namespace std;


// Genome

typedef const std::vector<double>& GENOME;

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

class Point {
public:
    double x, y;

    Point(double _x, double _y)
        : x(_x), y(_y)
    {}

    inline bool isInRect(Rect& r)
    {
        return x > r.x1 && y > r.y1 && x < r.x2 && y < r.y2;
    }
};


inline double getIntersectionArea(const Rect& r1, const Rect& r2)
{
    double area = (max(r1.x1, r2.x1) - min(r1.x2, r2.x2)) * (max(r1.y1, r2.y1) - min(r1.y2, r2.y2));
    return area > 0 ? area : 0;
}


// House

namespace house {
    int rooms;
    double space_width, space_height, out_wall, wall;
    bool** accesses;
    double* areas;

    // lights // 0: no light, 1: middle, 2: extreme
    int* lights;
    int space_light[4]; // clockwise // 0: up, 1: right, 2: down, 3: left

    double areaCoeff, intersectionCoeff, boundaryCoeff, proportionCoeff, accessCoeff, lightCoeff;

    // temporary
    vector<Point> points;
}
using namespace house;

void initHouse()
{
    // Space
    rooms = 7;

    wall = 0.15; out_wall = 0.3;
    space_width = 10.6; space_height = 10.05;
    space_width -= 2*out_wall - wall; space_height -= 2*out_wall - wall;
    space_light[0] = 2; space_light[1] = 1; space_light[2] = 0; space_light[3] = 0;


    // Areas
    areas = new double[rooms];
    // kitchen, bedroom1, bedroom2, bathroom, toilet, stairs, elevator
    areas[0] = 4; areas[1] = 4; areas[2] = 4; areas[3] = 1; areas[4] = 1; areas[5] = 4; areas[6] = 1;

    double sum = 0;
    for (int i = 0; i < rooms; i++)
        sum += areas[i];
    for (int i = 0; i < rooms; i++)
        areas[i] = areas[i] / (sum + 12);


    // Accesses
    accesses = new bool*[rooms];
    for (int i = 0; i < rooms; i++)
    {
        accesses[i] = new bool[rooms];
        for (int j = 0; j < rooms; j++)
             accesses[i][j] = 0;
    }

    // rooms are directly accessible from access space including living room and corridors
    accesses[5][6] = 1; // "stairs": ["elevator"]

    // Lights
    lights = new int[rooms];
    for (int i = 0; i < rooms; i++) lights[i] = 0;

    //lights[0] = 2; // livingroom
    lights[0] = 1; lights[1] = 1; lights[2] = 1; // kitchen, bedroom1, bedroom2


    // Distance Points
    const int size = 5;
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            points.push_back(Point(space_width/(size+1) * (i+1), space_height/(size+1) * (j+1)));


    // Coefficients
    areaCoeff = 1;
    intersectionCoeff = 2;
    boundaryCoeff = 2;
    proportionCoeff = 1;
    accessCoeff = 1;
    lightCoeff = 1;
}


// Penalty functions

double getAreaPenalty(GENOME genome, int index)
{
    static double space_area = space_width * space_height;
    return areaCoeff * 2 * sqrt(fabs(areas[index] * space_area - getArea(genome, index)));
}

double getProportionPenalty(GENOME genome, int index)
{
    double ratio = getWidth(genome, index) / getHeight(genome, index);
    if (ratio < 0)
        return 0; // invalid size - no penalty

    if (ratio > 1) ratio = 1 / ratio;

    if (ratio < 1 && ratio > 0.65)
        return 0; // good ratio

    return proportionCoeff * pow(2, 1/ratio);
}

double getBoundaryPenalty(GENOME genome, int index)
{
    static Rect r1(0, 0, space_width, space_height);
    Rect r2(genome, index);

    double l = r1.x1 - r2.x1, r = r2.x2 - r1.x2,
           u = r1.y1 - r2.y1, d = r2.y2 - r1.y2;

    return boundaryCoeff * ((l > 0 ? l : 0) + (r > 0 ? r : 0) + (u > 0 ? u : 0) + (d > 0 ? d : 0));
}

double getIntersectionPenalty(GENOME genome, int first, int second)
{
    const Rect &r1 = Rect(genome, first), &r2 = Rect(genome, second);

    double l = r1.x1 - r2.x2, r = r1.x2 - r2.x1,
           u = r1.y1 - r2.y2, d = r1.y2 - r2.y1;

    double minX = min(fabs(l), fabs(r)) * (l*r < 0 ? -1 : 1),
           minY = min(fabs(u), fabs(d)) * (u*d < 0 ? -1 : 1);

    return intersectionCoeff * (minX < 0 && minY < 0 ? min(fabs(minX), fabs(minY)) : 0);
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

        return accessCoeff * min(minX + yalign, minY + xalign);
    }

    return 0;
}

double getLightPenalty(GENOME genome, int index)
{
    if (! lights[index])
        return 0;

    static Rect space(0, 0, space_width, space_height);
    Rect room(genome, index);

    double d[4]; d[0] = room.y1 - space.y1; d[1] = space.x2 - room.x2; d[2] = space.y2 - room.y2; d[3] = room.x1 - space.x1;

    double minD = 100000, mini = -1;
    for (int i = 0; i < 4; i++)
        if (lights[index] <= space_light[i] && d[i] < minD)
        {
            minD = d[i];
            mini = i;
        }

    return lightCoeff * (mini >= 0 && minD > 0 ? minD : 0);
}

inline double getClsoest(vector<double>& vec, double value, bool previous)
{
    for (int i = 0; i <= rooms; i++)
        if (value < vec[i])
            return vec[i + (previous ? -1 : 0)];
}
double getDistancePenalty(GENOME genome)
{
    vector<double> ups, downs, lefts, rights;
    vector<Rect> rects;

    // space
    lefts.push_back(space_width);
    rights.push_back(0);
    ups.push_back(space_height);
    downs.push_back(0);

    // rooms
    for (int i = 0; i < rooms; i++)
    {
        rects.push_back(Rect(genome, i));

        lefts.push_back(rects[i].x1);
        rights.push_back(rects[i].x2);
        ups.push_back(rects[i].y1);
        downs.push_back(rects[i].y2);
    }

    sort(lefts.begin(), lefts.end());
    sort(rights.begin(), rights.end());
    sort(ups.begin(), ups.end());
    sort(downs.begin(), downs.end());


    double sum = 0, width, height;
    bool valid;
    for (int j, i = 0; i < points.size(); i++)
    {
        valid = true;
        for (j = 0; j < rooms; j++)
            if (points[i].isInRect(rects[j]))
                { valid = false; break; }
        if (! valid) continue;

        width = getClsoest(rights, points[i].x, true) - getClsoest(lefts, points[i].x, false);
        height = getClsoest(downs, points[i].y, true) - getClsoest(ups, points[i].y, false);

        sum += pow(min(width, height), 2);
    }

    return -sqrt(sum);
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
         penalty += getBoundaryPenalty(genome, i);
         penalty += getLightPenalty(genome, i);

         for (j = i+1; j < rooms; j++)
         {
             penalty += getIntersectionPenalty(genome, i, j);
             penalty += getAccessPenalty(genome, i, j);
         }
     }

    penalty += getDistancePenalty(genome);

    return penalty;
}

#endif
