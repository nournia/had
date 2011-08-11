
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

// House

namespace house {
    int rooms;
    double space_width, space_height, out_wall, wall;
    bool** accesses;
    double* areas;

    // lights // 0: no light, 1: middle, 2: extreme
    int* lights;
    int space_light[4]; // clockwise // 0: up, 1: right, 2: down, 3: left

    const double areaCoeff = 1, intersectionCoeff = 2, boundaryCoeff = 2, proportionCoeff = 1, accessCoeff = 1, lightCoeff = 1, spaceCoeff = 0.75, sideCoeff = 0.25;
}
using namespace house;

// Geometry
class Rect;
vector<Rect> rects;

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

    inline double getArea() const
    {
        return (x2 - x1) * (y2 - y1);
    }

    inline double getWidth()
    {
        return x2 - x1;
    }

    inline double getHeight()
    {
        return y2 - y1;
    }

    inline double getIntersectionArea(const Rect& r2, double offset = 0.01) const
    {
        double w = (max(x1, r2.x1) - min(x2, r2.x2)), h = (max(y1, r2.y1) - min(y2, r2.y2));
        double area = w < 0 && h < 0 ? w * h : 0;
        return area > offset ? area : 0;
    }

    inline bool isEmptyRect()
    {
        for (int i = 0; i < rects.size(); i++)
            if (getIntersectionArea(rects[i]) > 0)
                return false;
        return true;
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

    inline bool isEmptyPoint()
    {
        if (x < 0 || y < 0 || x > space_width || y > space_height)
            return false;

        for (int i = 0; i < rects.size(); i++)
            if (isInRect(rects[i]))
                return false;
        return true;
    }
};

// web
vector<Point> webPoints;
int webSize;


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


    // Web Points
    webSize = 10;
    for (int i = 0; i < webSize; i++)
        for (int j = 0; j < webSize; j++)
            webPoints.push_back(Point(space_width/(webSize+1) * (i+1), space_height/(webSize+1) * (j+1)));
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

inline bool findRect(vector<Rect>& rects, Rect& rect)
{
    for (int i = 0; i < rects.size(); i++)
        if (rects[i].x1 == rect.x1 && rects[i].x2 == rect.x2 && rects[i].y1 == rect.y1 && rects[i].y2 == rect.y2)
            return true;
    return false;
}

vector<Rect> getSpaces(GENOME genome)
{
    const double minSpaceLength = 1;
    vector<Rect> spaces;
//    vector<Point> points;

    // rooms
    rects.clear();
    for (int i = 0; i < rooms; i++)
    {
        Rect r(genome, i);
        rects.push_back(r);

//        points.push_back(Point((r.x1 + r.x2)/2, r.y1));
//        points.push_back(Point((r.x1 + r.x2)/2, r.y2));
//        points.push_back(Point(r.x1, (r.y1 + r.y2)/2));
//        points.push_back(Point(r.x2, (r.y1 + r.y2)/2));
    }


    for (int j, i = 0; i < webPoints.size(); i++)
    if (webPoints[i].isEmptyPoint())
    {
        double cleft = space_width, cright = 0, cup = space_height, cdown = 0;

        Point& point = webPoints[i];
        for (j = 0; j < rooms; j++)
        {
            Rect& rect = rects[j];

            if (point.y >= rect.y1 && point.y <= rect.y2)
            {
                if (point.x <= rect.x1 && cleft - point.x > rect.x1 - point.x) cleft = rect.x1;
                if (point.x >= rect.x2 && point.x - cright > point.x - rect.x2) cright = rect.x2;
            }

            if (point.x >= rect.x1 && point.x <= rect.x2)
            {
                if (point.y <= rect.y1 && cup - point.y > rect.y1 - point.y) cup = rect.y1;
                if (point.y >= rect.y2 && point.y - cdown > point.y - rect.y2) cdown = rect.y2;
            }
        }

        Rect r(cright, cdown, cleft, cup);
        if (r.isEmptyRect() && r.getWidth() > minSpaceLength &&  r.getHeight() > minSpaceLength &&  ! findRect(spaces, r))
            spaces.push_back(r);
    }

    return spaces;
}

double getSidePenalty(GENOME genome, int index)
{
    static Rect space(0, 0, space_width, space_height);
    Rect room(genome, index);

    double d[4]; d[0] = room.y1 - space.y1; d[1] = space.x2 - room.x2; d[2] = space.y2 - room.y2; d[3] = room.x1 - space.x1;
    double minD = 1000;
    for (int i = 0; i < 4; i++)
        if (fabs(d[i]) < minD)
            minD = fabs(d[i]);
    return sideCoeff * minD;
}

double getSpacePenalty(const vector<Rect>& spaces)
{
    vector<double> areas;
    for (int i = 0; i < spaces.size(); i++)
        areas.push_back(spaces[i].getArea());
    sort(areas.begin(), areas.end());

    double penalty = 0;

//    for (int i = 0; i < int(spaces.size()/3); i++)
//        penalty += sqrt(areas[i]);

    // minimize number of rectangles
//    penalty += spaces.size() > 1 ? (spaces.size() - 1) * 2 : 0;

//    // maximaize area of all spaces
    double sum;
//    for (int i = 0; i < areas.size(); i++)
//        sum += pow(areas[i], 2);
//    penalty -= sqrt(sqrt(sum));

    // biggest space
    int biggest = -1; double max = 0;
    for (int i = 0; i < spaces.size(); i++)
        if (spaces[i].getArea() > max)
            { max = spaces[i].getArea(); biggest = i; }

    if (biggest >= 0)
    {
        double intersection, area;

        // maximize access spaces area
        penalty -= 2 * sqrt(spaces[biggest].getArea());

        for (int i = 0; i < spaces.size(); i++)
        if (i != biggest)
        {
            intersection = spaces[biggest].getIntersectionArea(spaces[i]);
            area = spaces[i].getArea() - intersection;
            if (intersection > 0 && area > 0)
                sum += pow(area, 2);
        }
        penalty -= sqrt(sqrt(sum));


        // minimize biggest space distance from light sources
        static Rect space(0, 0, space_width, space_height);
        const Rect& room = spaces[biggest];

        double d[4]; d[0] = room.y1 - space.y1; d[1] = space.x2 - room.x2; d[2] = space.y2 - room.y2; d[3] = room.x1 - space.x1;
        for (int i = 0; i < 4; i++)
            penalty += d[i] * space_light[i];
    }

    return spaceCoeff * penalty;
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
         penalty += getSidePenalty(genome, i);

         for (j = i+1; j < rooms; j++)
         {
             penalty += getIntersectionPenalty(genome, i, j);
             penalty += getAccessPenalty(genome, i, j);
         }
     }

    if (penalty < 30)
    {
        const vector<Rect>& spaces = getSpaces(genome);
        penalty += getSpacePenalty(spaces);
    }

    return penalty;
}

#endif
