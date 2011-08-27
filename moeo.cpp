
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
    HAD() : moeoRealVector<HADObjectiveVector> (House::house->rooms) {}
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

eoGenOp<HAD> & do_make_op(eoParameterLoader& parser, eoState& state)
{
    double M_EPSILON = parser.createParam(0.01, "mutEpsilon", "epsilon for mutation",'e',"Param").value();
    double P_CROSS = parser.createParam(0.25, "pCross", "Crossover probability",'C',"Param").value();
    double P_MUT = parser.createParam(0.35, "pMut", "Mutation probability",'M',"Param").value();

    eoSequentialOp<HAD>* op = new eoSequentialOp<HAD>;
    state.storeFunctor(op);

    eoQuadCloneOp<HAD>* xover = new eoQuadCloneOp<HAD>;
    eoUniformMutation<HAD>* mutation = new eoUniformMutation<HAD>(M_EPSILON);

    state.storeFunctor(xover);
    state.storeFunctor(mutation);

    op->add(*xover, P_CROSS);
    op->add(*mutation, P_MUT);

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
    eoGenOp<HAD>& op = do_make_op(parser, state);

    // run
    // moeoNSGAII<HAD> algo (checkpoint, *eval, op);
    eoAlgo<HAD>& algo = do_make_ea_moeo(parser, state, *eval, checkpoint, op, arch);

    algo (pop);

    make_help(parser);
//    arch.sortedPrintOn (cout);
    return EXIT_SUCCESS;
}
