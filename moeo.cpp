
#include </home/alireza/repo/had/evaluate.h>

#include <es/eoRealInitBounded.h>
#include <es/eoRealOp.h>

#include <moeo>
#include <do/make_pop.h>
#include <do/make_continue_moeo.h>
#include <do/make_checkpoint_moeo.h>
#include <do/make_ea_moeo.h>
#include <do/make_run.h>

using namespace std;


class HADObjectiveVectorTraits : public moeoObjectiveVectorTraits {
public:
    static bool minimizing (int i) { return i < 4; }
    static bool maximizing (int i) { return i >= 4; }
    static unsigned int nObjectives () { return 6; }
};
typedef moeoRealObjectiveVector<HADObjectiveVectorTraits> HADObjectiveVector;

class HAD : public moeoRealVector<HADObjectiveVector> {
public:
    HAD() : moeoRealVector<HADObjectiveVector> (House::house->rooms * 4) {}
};

// evaluation of objective functions
class HADEval : public moeoEvalFunc<HAD>
{
public:
    void operator () (HAD& g)
    {
        if (g.invalidObjectiveVector())
        {
            HADObjectiveVector objVec;
            House* house = House::house;

            const int rooms = 4 * house->rooms;
            vector<double> genome(rooms);
            for (int i = 0; i < rooms; i++)
                genome[i] = g[i];

            house->update(genome);
            objVec[0] = house->getAreaPenalty();
            objVec[1] = house->getIntersectionPenalty();
            objVec[2] = house->getSidePenalty();

            house->updateSpaces();
            if (house->spaces.size() > 0)
            {
                objVec[3] = house->getAccessPenalty();
                objVec[4] = house->getSpaceProfit();
                objVec[5] = house->getLightProfit();
              }

            g.objectiveVector(objVec);
        }
    }
};


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
            mutEpsilon = parser.createParam(0.01, "mutEpsilon", "epsilon for mutation",'e',"Param").value(),
            pRoomSwapMut = parser.createParam(0.01, "pRoomSwapMut", "room swap mutation probability",'r',"Param").value();

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
    ptMon = new eoUniformMutation<EOT>(mutEpsilon);
    mutation = new eoPropCombinedMonOp<EOT>(*ptMon, 1); state.storeFunctor(ptMon);
    ptMon = new RoomSwapMutation<EOT>;
    mutation->add(*ptMon, pRoomSwapMut); state.storeFunctor(ptMon);

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


int main (int argc, char *argv[])
{
    eoParser parser(argc, argv);  // for user-parameter reading
    eoState state;                // to keep all things allocated

    // generate initial population
    eoRealVectorBounds bounds (House::house->rooms * 4, 0.0, 6.0);
    eoRealInitBounded<HAD>* init = new eoRealInitBounded<HAD>(bounds);
    state.storeFunctor(init);
    eoPop<HAD>& pop = do_make_pop(parser, state, *init);

    // problem independent
    eoEvalFuncCounter<HAD>* eval = new eoEvalFuncCounter<HAD>(*(new HADEval));
    state.storeFunctor(eval);

    moeoUnboundedArchive<HAD> arch;
    eoContinue<HAD>& term = do_make_continue_moeo(parser, state, *eval);
    eoCheckPoint<HAD>& checkpoint = do_make_checkpoint_moeo(parser, state, *eval, term, pop, arch);
    eoGenOp<HAD>& op = do_make_op(HAD(), parser, state);


    moeoAdditiveEpsilonBinaryMetric<HADObjectiveVector> indicator;
    // run
    moeoNSGAII<HAD> algo (checkpoint, *eval, op);
//    moeoIBEA<HAD> algo (checkpoint, *eval, op, indicator);
//    moeoNSGA<HAD> algo (checkpoint, *eval, op);
//    eoAlgo<HAD>& algo = do_make_ea_moeo(parser, state, *eval, checkpoint, op, arch); // moeoEasyEA
//    moeoSEEA2<HAD> algo (checkpoint, *eval, op, arch);
//    moeoSEEA<HAD> algo (checkpoint, *eval, op, arch);
//    moeoMOGA<HAD> algo (checkpoint, *eval, op);

    algo (pop);

    make_help(parser);
//    arch.sortedPrintOn (cout);
    return EXIT_SUCCESS;
}
