
#include <vector>
typedef const std::vector<double>& GENOME;

namespace house {
    int rooms;
    double width, height;
    bool** access;
    double* area;
}

double real_value(GENOME genome)
{
    double penalty = 0;

    for (int i = 0; i < 10; i++)
        penalty += genome[i];

    return penalty;
}
