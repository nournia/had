
#include <algorithm>
#include <string>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <time.h>

#include <eo>
#include <es/eoRealOp.h>
#include <es/make_genotype_real.cpp>
#include <es/make_pop_real.cpp>
#include <es/make_continue_real.cpp>
#include <es/make_checkpoint_real.cpp>
#include <es/make_run_real.cpp>
#include <do/make_algo_scalar.h>

using namespace std;

#include "/home/alireza/repo/had/evaluate.h"

#include <algo/moNeutralHC.h>


// Representation

template< class EOT >
class hadEval : public eoEvalFunc<EOT>
{
public:
    void operator()(EOT& _genome){
        _genome.fitness(real_value(_genome));
    }
};

typedef eoReal<eoMinimizingFitness> HAD;


// Neighbors

#include <eval/moFullEvalByModif.h>
#include <neighborhood/moOrderNeighborhood.h>

#include <neighborhood/moBackableNeighbor.h>
#include <neighborhood/moIndexNeighbor.h>

const size_t neighbors = 20;

struct Mem {
    int index;
    double diff;
};

template <class EOT, class Fitness=typename EOT::Fitness>
class hadRealNeighbor: public moBackableNeighbor<EOT, Fitness>, public moIndexNeighbor<EOT>
{
public:
    using moBackableNeighbor<EOT>::fitness;
    using moIndexNeighbor<EOT>::key;

    Mem* mem;

    int kIndex;
    double kDiff;

    hadRealNeighbor()
    {
        mem = new Mem[neighbors];
    }

    virtual void move(EOT& _solution)
    {
        // restart
        if (!key)
        {
            for (int i = 0; i < neighbors; i++)
                mem[i].index = -1;
            return;
        }

        setDiff();
        _solution[kIndex] += kDiff;
        _solution.invalidate();

//        cout << key << "\t" << kIndex << "\t" << real_value(_solution) << "\t" << kDiff << endl;
    }

    virtual void moveBack(EOT& _solution)
    {
        if (!key) return;

        setDiff();
        _solution[kIndex] -= kDiff;
        _solution.invalidate();
    }

    void setDiff()
    {
        if (mem[key].index == -1) // move
        {
            const size_t size = 4 * House::house->rooms;
            const double hcEpsilon = 0.5;

            kIndex = size_t(size * rng.uniform());
            kDiff = hcEpsilon * (rng.uniform() * 2 - 1);

            mem[key].index = kIndex;
            mem[key].diff = kDiff;
        }
        else // moveBack
        {
            kIndex = mem[key].index;
            kDiff = mem[key].diff;
        }
    }
};

typedef hadRealNeighbor<HAD> hadNeighbor;
typedef moOrderNeighborhood<hadNeighbor> orderNeighborhood;


// Operators

template<class GenotypeT>
class RoomExchangeCrossover: public eoQuadOp<GenotypeT>
{
    const double pCrossExchange;

public:
    RoomExchangeCrossover(double _pExchange = 0.1)
        : pCrossExchange(_pExchange)
    {}

    string className() const { return "RoomExchangeCrossover"; }

    // modifies both parents
    bool operator()(GenotypeT& g1, GenotypeT & g2)
    {
        bool oneAtLeastIsModified(false);

        double tmp;
        const size_t rooms = House::house->rooms;
        for (size_t t, j, i = 0; i < rooms; i++)
            if (rng.flip(pCrossExchange))
            {
                for (t = 4*i, j = 0; j < 4; j++)
                {
                    tmp = g1[t+j];
                    g1[t+j] = g2[t+j];
                    g2[t+j] = tmp;
                }

                if (!oneAtLeastIsModified) oneAtLeastIsModified = true;
            }

        return oneAtLeastIsModified;
    }
};

template<class GenotypeT>
class RoomSwapMutation: public eoMonOp<GenotypeT>
{
public:

    string className() const { return "RoomSwapMutation"; }

    // modifies parent
    bool operator()(GenotypeT& g)
    {
        bool isModified(false);

        const int rooms = House::house->rooms;
        size_t first = rooms * rng.uniform(), second = rooms * rng.uniform();
        first *= 4; second *= 4;

        if (first != second)
        {
            double  x1 = g[second] + (g[second+2] - g[first+2]) / 2,
                    y1 = g[second+1] + (g[second+3] - g[first+3]) / 2,
                    x2 = g[first] + (g[first+2] - g[second+2]) / 2,
                    y2 = g[first+1] + (g[first+3] - g[second+3]) / 2;

            g[first] = x1; g[first+1] = y1;
            g[second] = x2; g[second+1] = y2;

            isModified = true;
        }

        return isModified;
    }
};

template <class EOT>
eoGenOp<EOT> & do_make_op(EOT, eoParser& parser, eoState& state)
{
    double  pCross = parser.createParam(0.1, "pCross", "Crossover probability",'C',"Param").value(),
            pRoomExchangeCross = parser.createParam(0.1, "pRoomExchangeCross", "Room exchange probability in Crossover",'E',"Param").value(),

            pMut = parser.createParam(1, "pMut", "Mutation probability",'M',"Param").value(),
            pUniformMut = parser.createParam(0.1, "pUniformMut", "Uniform mutation probability",'M',"Param").value(),
            eUniformMut = parser.createParam(1, "eUniformMut", "epsilon for uniform mutation",'e',"Param").value(),
            pRoomSwapMut = parser.createParam(0.01, "pRoomSwapMut", "room swap mutation probability",'r',"Param").value(),
            pLocalSearchMut = parser.createParam(0.01, "pLocalSearchMut", "local search mutation probability",'l',"Param").value(),
            maxLocalSearchStep = parser.createParam(50, "maxLocalSearchStep", "maximum steps of local search operator",'h',"Param").value();

    eoQuadOp<EOT> *ptQuad; // tmp
    eoPropCombinedQuadOp<EOT>* xover;
    state.storeFunctor(xover);
    eoPropCombinedMonOp<EOT>* mutation;
    eoMonOp<EOT> *ptMon;
    state.storeFunctor(mutation);


    // xover
    ptQuad = new RoomExchangeCrossover<EOT>(pRoomExchangeCross);
    xover = new eoPropCombinedQuadOp<EOT>(*ptQuad, 1); state.storeFunctor(ptQuad);


    // mutation
    ptMon = new eoUniformMutation<EOT>(eUniformMut);
    mutation = new eoPropCombinedMonOp<EOT>(*ptMon, pUniformMut); state.storeFunctor(ptMon);

    ptMon = new RoomSwapMutation<EOT>;
    mutation->add(*ptMon, pRoomSwapMut); state.storeFunctor(ptMon);

    // hc
    hadEval<EOT>* fullEval = new hadEval<EOT>; state.storeFunctor(fullEval);
    moFullEvalByModif<hadNeighbor>* neighborEval = new moFullEvalByModif<hadNeighbor>(*fullEval); state.storeFunctor(neighborEval);
    orderNeighborhood* neighborhood = new orderNeighborhood(neighbors);
    ptMon = new moNeutralHC<hadNeighbor>(*neighborhood, *fullEval, *neighborEval, maxLocalSearchStep);
    mutation->add(*ptMon, pLocalSearchMut); state.storeFunctor(ptMon);


    // a proportional combination of a QuadCopy and crossover
    eoProportionalOp<EOT>* cross = new eoProportionalOp<EOT> ; state.storeFunctor(cross);
    ptQuad = new eoQuadCloneOp<EOT>; state.storeFunctor(ptQuad);
    cross->add(*xover, pCross); // xover
    cross->add(*ptQuad, 1-pCross); // clone

    // now the sequential
    eoSequentialOp<EOT> *op = new eoSequentialOp<EOT>; state.storeFunctor(op);
    op->add(*cross, 1.0);  // always crossover (but clone with prob 1-pCross clone)
    op->add(*mutation, pMut);

    return *op;
}


// Algorithm

template <class EOT>
void runAlgorithm(EOT, eoParser& _parser, eoState& _state);

int main_function(int argc, char *argv[])
{
    eoParser parser(argc, argv); // for user-parameter reading
    eoState state; // keeps all things allocated

    runAlgorithm(HAD(), parser, state);

    return 0;
}

// A main that catches the exceptions
int main(int argc, char **argv)
{
    try
    {
        main_function(argc, argv);
    }
    catch(exception& e)
    {
        cout << "Exception: " << e.what() << '\n';
    }

    return 1;
}

template <class EOT>
void runAlgorithm(EOT, eoParser& _parser, eoState& _state)
{
    typedef typename EOT::Fitness FitT;

    // The evaluation fn - encapsulated into an eval counter for output
    eoEvalFuncPtr<EOT, double, const std::vector<double>&> mainEval( real_value );
    eoEvalFuncCounter<EOT> eval(mainEval);

    eoRealInitBounded<EOT>& init = make_genotype(_parser, _state, EOT());

    eoGenOp<EOT>& op = do_make_op(EOT(), _parser, _state);

    // initialize the population - and evaluate
    eoPop<EOT>& pop = make_pop(_parser, _state, init);
    apply<EOT>(eval, pop);

    eoContinue<EOT> & term = make_continue(_parser, _state, eval);
    eoCheckPoint<EOT> & checkpoint = make_checkpoint(_parser, _state, eval, term);
    eoAlgo<EOT>& ga = do_make_algo_scalar(_parser, _state, eval, checkpoint, op);

    run_ea(ga, pop);

    make_help(_parser);
    // pop.sortedPrintOn(cout);
}
