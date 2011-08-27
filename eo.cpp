
#include <algorithm>
#include <string>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <time.h>

#include <eo>
#include <es/make_es.h>
#include <eoRealOp.h>

using namespace std;

#include "/home/alireza/repo/had/evaluate.h"


template<class GenotypeT>
class RoomExchangeCrossover: public eoQuadOp<GenotypeT>
{
    const double pExchange;
public:
    RoomExchangeCrossover(double _pExchange = 0.1)
        : pExchange(_pExchange)
    {}

    string className() const { return "RoomExchangeCrossover"; }

    // eoQuad crossover - modifies both parents
    bool operator()(GenotypeT& g1, GenotypeT & g2)
    {
        bool oneAtLeastIsModified(false);

        double tmp;
        const size_t rooms = House::house->rooms;
        for (size_t t, j, i = 0; i < rooms; i++)
            if (rng.flip(pExchange))
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


template <class EOT>
eoGenOp<EOT> & do_make_op(EOT, eoParser& parser, eoState& state)
{
    eoRealVectorBounds* ptBounds = new eoRealVectorBounds(House::house->rooms * 4, 0, 10);

    double P_CROSS = parser.createParam(0.6, "pCross", "Crossover probability",'C',"Param").value();
    double P_EXCHANGE = parser.createParam(0.6, "pExchange", "Room exchange probability in Crossover",'E',"Param").value();
    double P_MUT = parser.createParam(0.1, "pMut", "Mutation probability",'M',"Param").value();
    double M_EPSILON = parser.createParam(0.01, "mutEpsilon", "epsilon for mutation",'e',"Param").value();

    eoQuadOp<EOT> *ptQuad; // tmp
    eoPropCombinedQuadOp<EOT>* xover;
    state.storeFunctor(xover);
    eoPropCombinedMonOp<EOT>* mutation;
    eoMonOp<EOT> *ptMon;
    state.storeFunctor(mutation);

    // xover
    ptQuad = new RoomExchangeCrossover<EOT>(P_EXCHANGE); state.storeFunctor(ptQuad);
    xover = new eoPropCombinedQuadOp<EOT>(*ptQuad, 1);

    // mutation
    ptMon = new eoUniformMutation<EOT>(*ptBounds, M_EPSILON); state.storeFunctor(ptMon);
    mutation = new eoPropCombinedMonOp<EOT>(*ptMon, 1);

    // a proportional combination of a QuadCopy and crossover
    eoProportionalOp<EOT>* cross = new eoProportionalOp<EOT> ; state.storeFunctor(cross);
    ptQuad = new eoQuadCloneOp<EOT>; state.storeFunctor(ptQuad);
    cross->add(*xover, P_CROSS); // xover
    cross->add(*ptQuad, 1-P_CROSS); // clone

    // now the sequential
    eoSequentialOp<EOT> *op = new eoSequentialOp<EOT>; state.storeFunctor(op);
    op->add(*cross, 1.0);  // always crossover (but clone with prob 1-pCross clone)
    op->add(*mutation, P_MUT);

    return *op;
}



// Now the main
///////////////
typedef eoMinimizingFitness  FitT;

template <class EOT>
void runAlgorithm(EOT, eoParser& _parser, eoState& _state);

int main_function(int argc, char *argv[])
{
    // Create the command-line parser
    eoParser parser(argc, argv); // for user-parameter reading
    eoState state; // keeps all things allocated

    eoValueParam<bool>& simpleParam = parser.createParam(true, "Isotropic", "Isotropic self-adaptive mutation", 'i', "ES mutation");
    eoValueParam<bool>& stdevsParam = parser.createParam(false, "Stdev", "One self-adaptive stDev per variable", 's', "ES mutation");
    eoValueParam<bool>& corrParam = parser.createParam(false, "Correl", "Use correlated mutations", 'c', "ES mutation");

    // Run the appropriate algorithm
    if (simpleParam.value() == false)
    {
        cout << "Using eoReal" << endl;
        runAlgorithm(eoReal<FitT>(), parser, state);
    }
    else if (stdevsParam.value() == false)
    {
        cout << "Using eoEsSimple" << endl;
        runAlgorithm(eoEsSimple<FitT>(), parser, state);
    }
    else if (corrParam.value() == false)
    {
        cout << "Using eoEsStdev" << endl;
        runAlgorithm(eoEsStdev<FitT>(), parser, state);
    }
    else
    {
        cout << "Using eoEsFull" << endl;
        runAlgorithm(eoEsFull<FitT>(), parser, state);
    }

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
    //  eoGenOp<EOT>& op = make_op(_parser, _state, init);

    // initialize the population - and evaluate
    eoPop<EOT>& pop = make_pop(_parser, _state, init);
    apply<EOT>(eval, pop);

    eoContinue<EOT> & term = make_continue(_parser, _state, eval);
    eoCheckPoint<EOT> & checkpoint = make_checkpoint(_parser, _state, eval, term);
    eoAlgo<EOT>& ga = make_algo_scalar(_parser, _state, eval, checkpoint, op);

    run_ea(ga, pop);

    make_help(_parser);
    // pop.sortedPrintOn(cout);
}

