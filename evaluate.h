
#ifndef EVALUATION_H
#define EVALUATION_H

#include <algorithm>
#include <vector>
#include <math.h>
using namespace std;

typedef const std::vector<double>& GENOME;

const double areaCoeff = 2.5, intersectionCoeff = 3, boundaryCoeff = 3, accessCoeff = 2, lightCoeff = 1, spaceCoeff = 0.75, sideCoeff = 0;


// Geometry

class Rect {
public:
    double x1, y1, x2, y2;

    Rect()
    {}

    Rect(double _x1, double _y1, double _x2, double _y2)
        :x1(_x1), y1(_y1), x2(_x2), y2(_y2)
    {}

    void set(double _x1, double _y1, double _x2, double _y2)
    {
        x1 = _x1; y1 = _y1; x2 = _x2; y2 = _y2;
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

class Size {
public:
    double width, height;
};

// Temporary

inline double areaToDistance(double area)
{
    return sqrt(fabs(area));
}

inline bool findRect(vector<Rect>& rects, Rect& rect)
{
    for (int i = 0; i < rects.size(); i++)
        if (rects[i].x1 == rect.x1 && rects[i].x2 == rect.x2 && rects[i].y1 == rect.y1 && rects[i].y2 == rect.y2)
            return true;
    return false;
}

void addRectanglePoints(vector<Point>& points, Rect& r)
{
    const int parts = 3;
    double xdiff = (r.x2 - r.x1) / (parts + 1), ydiff = (r.y2 - r.y1) / (parts + 1);

    for (int i = 1; i <= parts; i++)
    {
        points.push_back(Point(r.x1 + i*xdiff, r.y1));
        points.push_back(Point(r.x1 + i*xdiff, r.y2));
        points.push_back(Point(r.x1, r.y1 + i*ydiff));
        points.push_back(Point(r.x2, r.y1 + i*ydiff));
    }
}

// Problem

class Room {
public:
    int lightLimit; // lights // 0: no light, 1: middle, 2: extreme
    double areaLimit;
    Size sizeLimit;
    Rect rect;

    Room()
    {
        // no default limit
        lightLimit = 0;
        areaLimit = 0;
        sizeLimit.width = 0;
        sizeLimit.height = 0;
    }
};

class House {
public:
    size_t rooms;
    vector<Room> room;
    Rect space;
    double out_wall, wall;
    int* access;
    int light[4]; // clockwise // 0: up, 1: right, 2: down, 3: left

    static House* house;

    House()
    {
        // Space
        wall = 0.15; out_wall = 0.3;
        space.x1 = 0; space.y1 = 0; space.x2 = 10.6; space.y2 = 10.05;
        space.x2 -= 2*out_wall - wall; space.y2 -= 2*out_wall - wall;
        light[0] = 2; light[1] = 1; light[2] = 0; light[3] = 0;


        // kitchen, bedroom1, bedroom2, bathroom, toilet, stairs, elevator
        rooms = 7;
        for (int i = 0; i < rooms; i++)
            room.push_back(Room());

        // Area
        room[5].sizeLimit.height = 2.5; room[5].sizeLimit.width = 4.5; // stairs
        room[6].sizeLimit.height = 1.6; room[6].sizeLimit.width = 2; // elevator

        double emptySpace = space.getWidth() * space.getHeight();
        for (int i = 0; i < rooms; i++)
            if (room[i].sizeLimit.width)
                emptySpace -= (room[i].sizeLimit.height + wall) * (room[i].sizeLimit.width + wall);

        const double all = 3*4 + 2*1 + 12;
        room[0].areaLimit = emptySpace * 4/all;
        room[1].areaLimit = emptySpace * 4/all;
        room[2].areaLimit = emptySpace * 4/all;
        room[3].areaLimit = emptySpace * 1/all;
        room[4].areaLimit = emptySpace * 1/all;


        // Access
        access = new int[rooms];
        for (int i = 0; i < rooms; i++)
            access[i] = -1; // rooms are directly accessible from access space including living room and corridors
        access[6] = 5; // "stairs": ["elevator"]

        // Lights
        room[0].lightLimit = 1; room[1].lightLimit = 1; room[2].lightLimit = 1; // kitchen, bedroom1, bedroom2

    }

    void update(GENOME genome)
    {
        // rooms
        for (int i = 0; i < rooms; i++)
        {
            int index = 4 * i;
            room[i].rect.set(genome[index], genome[index+1], genome[index] + genome[index+2], genome[index+1] + genome[index+3]);
        }
    }

    vector<Rect> spaces;
    void updateSpaces()
    {
        // points
        vector<Point> points;
        for (int i = 0; i < rooms; i++)
            addRectanglePoints(points, room[i].rect);

        // spaces
        vector<Rect> tmps;
        const double minSpaceLength = 1;
        for (int j, i = 0; i < points.size(); i++)
        if (isEmptyPoint(points[i]))
        {
            double cleft = space.getWidth(), cright = 0, cup = space.getHeight(), cdown = 0;

            Point& point = points[i];
            for (j = 0; j < rooms; j++)
            {
                Rect& rect = room[j].rect;

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
            if (isEmptyRect(r) && r.getWidth() > minSpaceLength &&  r.getHeight() > minSpaceLength &&  ! findRect(tmps, r))
                tmps.push_back(r);
        }

        // find biggest space
        int biggest = -1; double max = 0;
        for (int i = 0; i < tmps.size(); i++)
            if (tmps[i].getArea() > max)
                { max = tmps[i].getArea(); biggest = i; }

        // find access spaces
        spaces.clear();
        spaces.push_back(tmps[biggest]);
        for (int i = 0; i < tmps.size(); i++)
        if (i != biggest)
            if (spaces[0].getIntersectionArea(tmps[i]) > 0)
                spaces.push_back(tmps[i]);
    }


    // Geometry funcitons

    inline bool isEmptyPoint(Point& p)
    {
        if (! p.isInRect(space))
            return false;

        for (int i = 0; i < rooms; i++)
            if (p.isInRect(room[i].rect))
                return false;
        return true;
    }

    inline bool isEmptyRect(Rect& r)
    {
        for (int i = 0; i < rooms; i++)
            if (r.getIntersectionArea(room[i].rect) > 0)
                return false;
        return true;
    }

    inline double getAlign(double r11, double r12, double r21, double r22)
    {
        const int offset = 1; // door width

        double align = 0;
        double a1 = r11 + offset, a2 = r12 - offset;
        if (r22 < a1) align = a1 - r22;
        if (r21 > a2) align = r21 - a2;
        if (r22 < a1 && r21 > a2) align = 0;
        return align;
    }

    inline double getAccessDistance(Rect& r1, Rect& r2)
    {
        double l = r1.x1 - r2.x2, r = r1.x2 - r2.x1,
               u = r1.y1 - r2.y2, d = r1.y2 - r2.y1;

        double minX = min(fabs(l), fabs(r)),
               minY = min(fabs(u), fabs(d));

        double xalign = getAlign(r1.x1, r1.x2, r2.x1, r2.x2),
               yalign = getAlign(r1.y1, r1.y2, r2.y1, r2.y2);

        return min(minX + yalign, minY + xalign);
    }

    inline double* getSideDistances(const Rect& room)
    {
        double* d = new double[4];
        d[0] = room.y1 - space.y1; d[1] = space.x2 - room.x2; d[2] = space.y2 - room.y2; d[3] = room.x1 - space.x1;
        return d;
    }

    // Penalty functions

    double getAreaPenalty(int index)
    {
        double penalty = 0;

        if (room[index].sizeLimit.width)
        {
            double w1 = room[index].rect.getWidth() - wall, h1 = room[index].rect.getHeight() - wall,
                   w2 = room[index].sizeLimit.width, h2 = room[index].sizeLimit.height;

            penalty += min(fabs(w2 - w1) + fabs(h2 - h1), fabs(w2 - h1) + fabs(h2 - w1));

        } else
        {
            penalty += areaToDistance(room[index].areaLimit - room[index].rect.getArea());
            penalty += getProportionPenalty(index);
        }

        return areaCoeff * penalty;
    }

    double getProportionPenalty(int index)
    {
        double ratio = room[index].rect.getWidth() / room[index].rect.getHeight();
        if (ratio < 0)
            return 0; // invalid size - no penalty

        if (ratio > 1) ratio = 1 / ratio;

        if (ratio < 1 && ratio > 0.6)
            return 0; // good ratio

        return pow(2, 1/ratio);
    }

    double getBoundaryPenalty(int index)
    {
        Rect& r1 = space;
        Rect& r2 = room[index].rect;

        double l = r1.x1 - r2.x1, r = r2.x2 - r1.x2,
               u = r1.y1 - r2.y1, d = r2.y2 - r1.y2;

        return boundaryCoeff * ((l > 0 ? l : 0) + (r > 0 ? r : 0) + (u > 0 ? u : 0) + (d > 0 ? d : 0));
    }

    double getSidePenalty(int index)
    {
        double* d = getSideDistances(room[index].rect);
        double minD = 1000;
        for (int i = 0; i < 4; i++)
            if (fabs(d[i]) < minD)
                minD = fabs(d[i]);
        return sideCoeff * minD;
    }

    double getLightPenalty(int index)
    {
        if (! room[index].lightLimit)
            return 0;

        double* d = getSideDistances(room[index].rect);
        double minD = 100000, mini = -1;
        for (int i = 0; i < 4; i++)
            if (room[index].lightLimit <= light[i] && d[i] < minD)
            {
                minD = d[i];
                mini = i;
            }

        return lightCoeff * (mini >= 0 && minD > 0 ? minD : 0);
    }

    double getIntersectionPenalty(int first, int second)
    {
        const Rect &r1 = room[first].rect, &r2 = room[second].rect;

        double l = r1.x1 - r2.x2, r = r1.x2 - r2.x1,
               u = r1.y1 - r2.y2, d = r1.y2 - r2.y1;

        double minX = min(fabs(l), fabs(r)) * (l*r < 0 ? -1 : 1),
               minY = min(fabs(u), fabs(d)) * (u*d < 0 ? -1 : 1);

        return intersectionCoeff * (minX < 0 && minY < 0 ? min(fabs(minX), fabs(minY)) : 0);
    }

    double getAccessPenalty()
    {
        double penalty = 0;
        for (int j, i = 0; i < rooms; i++)
            if (access[i] < 0) // from access space
            {
                double dist, min = 10000;
                for (j = 0; j < spaces.size(); j++)
                {
                    dist = getAccessDistance(room[i].rect, spaces[j]);
                    if (dist < min) min = dist;
                    if (min < 0.01) break;
                }
                penalty += min;
            }
            else // from another room
                penalty += getAccessDistance(room[i].rect, room[access[i]].rect);

        return accessCoeff * penalty;
    }

    double getSpacePenalty()
    {
        if (spaces.size() == 0) return 0;
        double penalty = 0;

        // Area

//        vector<double> areas;
//        for (int i = 0; i < spaces.size(); i++)
//            areas.push_back(spaces[i].getArea());
//        sort(areas.begin(), areas.end());

//        // minimize area of 1/3 of spaces that are small ones.
//        for (int i = 0; i < int(spaces.size()/3); i++)
//            penalty += sqrt(areas[i]);

//        // minimize number of rectangles and maximaize area of all spaces
//        penalty += spaces.size() > 1 ? (spaces.size() - 1) * 1 : 0;


        // maximize access spaces area
        penalty -= 2 * sqrt(spaces[0].getArea());

        double intersection, area, sum;
        for (int i = 1; i < spaces.size(); i++)
        {
            intersection = spaces[0].getIntersectionArea(spaces[i]);
            area = spaces[i].getArea() - intersection;
            if (intersection > 0 && area > 0)
                sum += area;
        }
        penalty -= areaToDistance(sum);


        // Light

        // minimize biggest space distance from light sources
        double* d = getSideDistances(spaces[0]);
        for (int i = 0; i < 4; i++)
            penalty += d[i] * light[i];

        return spaceCoeff * penalty;
    }
};
House* House::house = new House;


// Evaluate

double real_value(GENOME genome)
{
    House* house = House::house;

    house->update(genome);
    double penalty = 0;
    for (int j, i = 0; i < house->rooms; i++)
     {
         penalty += house->getAreaPenalty(i);
         penalty += house->getBoundaryPenalty(i);
         penalty += house->getSidePenalty(i);
         penalty += house->getLightPenalty(i);

         for (j = i+1; j < house->rooms; j++)
             penalty += house->getIntersectionPenalty(i, j);
     }

    if (penalty > 40)
        return penalty;

    house->updateSpaces();

    penalty += house->getSpacePenalty();
    penalty += house->getAccessPenalty();

    return penalty;
}

#endif
