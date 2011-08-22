
#ifndef EVALUATION_H
#define EVALUATION_H

#include <algorithm>
#include <vector>
#include <math.h>
using namespace std;

typedef const std::vector<double>& GENOME;

const double areaCoeff = 3, intersectionCoeff = 3, accessCoeff = 1.5, lightCoeff = 0.25, spaceCoeff = 0.75, sideCoeff = 0.25;


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
        if (fabs(rect.getIntersectionArea(rects[i]) - rect.getArea()) < 1)
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
    double original_width, original_height;
    double out_wall, wall;
    int* access;
    int light[4]; // clockwise // 0: up, 1: right, 2: down, 3: left

    static House* house;

    House()
    {
        original_width = 10.6;
        original_height = 10.05;

        // Space
        wall = 0.15; out_wall = 0.3;
        space.x1 = 0; space.y1 = 0; space.x2 = original_width; space.y2 = original_height;
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

        const double all = 3*4 + 2*1 + 15;
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
        static vector<Point> points;

//        for (int i = 0; i < rooms; i++)
//            addRectanglePoints(points, room[i].rect);

        if (points.size() == 0)
        {
            const int webSize = 12;
            for (int j, i = 0; i < webSize; i++)
                for (j = 0; j < webSize; j++)
                    points.push_back(Point(space.getWidth()/(webSize+1) * (i+1), space.getHeight()/(webSize+1) * (j+1)));
        }

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
            emptyRect(r);

            if (r.getWidth() > minSpaceLength &&  r.getHeight() > minSpaceLength && ! findRect(tmps, r))
                tmps.push_back(r);
        }

        // find biggest space
        int fittest = -1; double side, max = 0;
        for (int i = 0; i < tmps.size(); i++)
        {
            side = min(tmps[i].getWidth(), tmps[i].getHeight());
            if (side > max)
                { max = side; fittest = i; }
        }

        // find access spaces
        spaces.clear();
        if (tmps.size() == 0) return;

        spaces.push_back(tmps[fittest]);
        for (int i = 0; i < tmps.size(); i++)
        if (i != fittest)
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

    inline void emptyRect(Rect& r)
    {
        Rect r1, r2;
        for (int i = 0; i < rooms; i++)
            if (r.getIntersectionArea(room[i].rect) > 0)
            {
                r1 = r; r2 = r;
                Rect& rm = room[i].rect;
                if (r.x1 < rm.x1) r1.x2 = rm.x1; else r1.x1 = rm.x2;
                if (r.y1 < rm.y1) r2.y2 = rm.y1; else r2.y1 = rm.y2;

                r = r1.getArea() > r2.getArea() ? r1 : r2;
            }
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
        double* d = new double[4]; // 0: up, 1: right, 2: down, 3: left
        d[0] = fabs(room.y1 - space.y1); d[1] = fabs(space.x2 - room.x2); d[2] = fabs(space.y2 - room.y2); d[3] = fabs(room.x1 - space.x1);
        return d;
    }

    // Penalty functions

    double getAreaPenalty()
    {
        double penalty = 0;

        for (int i = 0; i < rooms; i++)
            if (room[i].sizeLimit.width)
            {
                double w1 = room[i].rect.getWidth() - wall, h1 = room[i].rect.getHeight() - wall,
                       w2 = room[i].sizeLimit.width, h2 = room[i].sizeLimit.height;

                double wd, hd;
                if (fabs(w2 - w1) + fabs(h2 - h1) < fabs(w2 - h1) + fabs(h2 - w1))
                {
                    wd = w2 - w1; hd = h2 - h1;
                } else
                {
                    wd = w2 - h1; hd = h2 - w1;
                }

                penalty += exp((wd > 0 ? wd : 0) + (hd > 0 ? hd : 0));

            } else
            {
                const double minRatio = 0.7, iMinRatio = 1.0 / minRatio;

                double w = room[i].rect.getWidth(), h = room[i].rect.getHeight();
                if (w > h)
                {
                    w = room[i].rect.getHeight(); h = room[i].rect.getWidth();
                }

                if (h > (w * iMinRatio))
                    h = w * iMinRatio;
                else
                    w = h * minRatio;

                double ad = room[i].areaLimit - (w * h);
                penalty += ad > 0 ? 2 * areaToDistance(ad) : 0;
            }

        return areaCoeff * penalty;
    }


    double getBoundaryIntersection(int index)
    {
        Rect &r1 = space, &r2 = room[index].rect;

        double l = r1.x1 - r2.x1, r = r2.x2 - r1.x2,
               u = r1.y1 - r2.y1, d = r2.y2 - r1.y2;

        return (l > 0 ? l : 0) + (r > 0 ? r : 0) + (u > 0 ? u : 0) + (d > 0 ? d : 0);
    }
    double getRoomIntersection(int first, int second)
    {
        const Rect &r1 = room[first].rect, &r2 = room[second].rect;

        double l = r1.x1 - r2.x2, r = r1.x2 - r2.x1,
               u = r1.y1 - r2.y2, d = r1.y2 - r2.y1;

        double minX = min(fabs(l), fabs(r)) * (l*r < 0 ? -1 : 1),
               minY = min(fabs(u), fabs(d)) * (u*d < 0 ? -1 : 1);

        return minX < 0 && minY < 0 ? min(fabs(minX), fabs(minY)) : 0;
    }
    double getIntersectionPenalty()
    {
        double penalty = 0;
        for (int j, i = 0; i < rooms; i++)
        {
            penalty += house->getBoundaryIntersection(i);
            for (j = i+1; j < house->rooms; j++)
                penalty += house->getRoomIntersection(i, j);
        }

        return intersectionCoeff * penalty;
    }

    double getSidePenalty()
    {
        double penalty = 0, *dists;
        for (int i = 0; i < rooms; i++)
        {
            dists = getSideDistances(room[i].rect);
            penalty += min(dists[0], dists[2]) + min(dists[1], dists[3]);
        }

        return sideCoeff * penalty;
    }

    double getRoomLight(Rect& rect, int lightLimit)
    {
        const double lightDistanceLimit = 0.5;

        double sum = 0;
        double* dists = getSideDistances(rect);
        for (int i = 0; i < 4; i++)
            if (light[i] &&  dists[i] < lightDistanceLimit)
                sum += lightLimit * light[i] * (!(i%2) ? rect.getWidth() : rect.getHeight());

        return sum;
    }
    double getLightProfit()
    {
        double profit = 0;

        for (int j, i = 0; i < rooms; i++)
        if (room[i].lightLimit)
            profit += getRoomLight(room[i].rect, 1);

        profit += getRoomLight(spaces[0], 2);

        return lightCoeff * profit;
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

    double getSpaceProfit()
    {
        double profit = 0;

        // minimize number of rectangles and maximaize area of all spaces
        profit -= spaces.size() > 1 ? (spaces.size() - 1) * 1 : 0;

        // maximize access spaces area
        profit += 2 * sqrt(spaces[0].getArea());

        double intersection, area, sum;
        for (int i = 1; i < spaces.size(); i++)
        {
            intersection = spaces[0].getIntersectionArea(spaces[i]);
            area = spaces[i].getArea() - intersection;
            if (intersection > 0 && area > 0)
                sum += area;
        }
        profit += areaToDistance(sum);

        return spaceCoeff * profit;
    }
};
House* House::house = new House;


// Evaluate

double real_value(GENOME genome)
{
    House* house = House::house;
    house->update(genome);

    double penalty = 0;

    penalty += house->getAreaPenalty();
    penalty += house->getSidePenalty();
    penalty += house->getIntersectionPenalty();

    house->updateSpaces();
    if (house->spaces.size() > 0)
    {
        penalty -= house->getSpaceProfit();
        penalty -= house->getLightProfit();
        penalty += house->getAccessPenalty();
    }

    return penalty;
}

#endif
