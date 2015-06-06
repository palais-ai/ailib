#ifndef GENETIC_H
#define GENETIC_H

#include "ai_global.h"
#include <vector>
#include <sparsehash/sparse_hash_map>
#include <cstdlib>
#include <cmath>

BEGIN_NS_AILIB

class GeneticListener
{
public:
    virtual void onGeneration(uint32_t generation, real_type bestFitness) = 0;
};

template <typename T>
struct Hash;

/**
 * @brief The Genetic class implements a Genetic Algorithm with crossover, mutation and elitism.
 * The problem-specific crossover, mutation, generator and fitness functions must be supplied
 * by the user.
 */
template <typename T, typename HASH_FUN = Hash<T> >
class Genetic
{
    GeneticListener* mListener;
public:
    typedef google::sparse_hash_map<T, real_type, HASH_FUN> hash_type;
    typedef real_type(*FitnessFunction  )(const T&);
    typedef         T(*CrossoverFunction)(const T&, const T&);
    typedef         T(*MutationFunction )(const T&);
    typedef         T(*GeneratorFunction)(uint32_t);
private:

    class FitnessComparator
    {
        // TODO: Add a HashingPolicy template type parameter.
        //mutable hash_type mHashMap;
        FitnessFunction mFitness;
    public:
        FitnessComparator(FitnessFunction f) :
            mFitness(f)
        {
            ;
        }

        real_type getFitnessForValue(const T& value) const
        {
            real_type fitness = mFitness(value);
            /*
            typename hash_type::iterator it = mHashMap.find(value);
            if(it == mHashMap.end())
            {
                mHashMap[value] = fitness = mFitness(value);
            }
            else
            {
                fitness = (*it).second;
            }*/
            return fitness;
        }

        FORCE_INLINE bool operator()(const T& lv, const T& rv) const
        {
            return getFitnessForValue(lv) < getFitnessForValue(rv); // Sort from lowest to highest
        }
    };
public:

    Genetic(FitnessFunction ff,
            CrossoverFunction cf,
            MutationFunction mf,
            GeneratorFunction gf,
            uint32_t populationSize) :
        mListener(NULL),
        mPopulation(populationSize),
        mBacking(populationSize),
        mFitness(ff),
        mCrossover(cf),
        mMutation(mf),
        mGenerator(gf)
    {
        ;
    }

    void setListener(GeneticListener* listener)
    {
        mListener = listener;
    }

    void generatePopulation()
    {
        for(size_t i = 0; i < mPopulation.size(); ++i)
        {
            mPopulation[i] = mGenerator(i);
        }
    }

    T optimise(uint32_t numGenerations,
               real_type pElitism,
               real_type pCrossover,
               real_type pMutation)
    {
        FitnessComparator comp(mFitness);
        // A full sort is not completely necessary.
        // An alternative strategy is to pick out a few chromosomes and keep the best ones.
        std::sort(mPopulation.begin(),
                  mPopulation.end(),
                  comp);

        if(mListener)
        {
            mListener->onGeneration(0, mFitness(mPopulation[0]));
        }

        for(size_t gens = 0; gens < numGenerations; ++gens)
        {
            // Update the shadow copy
            std::copy(mPopulation.begin(),
                      mPopulation.end(),
                      mBacking.begin());

            const uint32_t numElitists = pElitism * mPopulation.size();
            for(int64_t i = mPopulation.size() - 1; i > mPopulation.size() * pCrossover; --i)
            {
                uint32_t first = randRangeExp(numElitists, mPopulation.size() - 1, 0.4);
                uint32_t second = 0;
                do
                {
                    second = randRangeExp(0, mPopulation.size(), 0.4);
                } while(first == second);

                mPopulation[i] = mCrossover(mBacking[first], mBacking[second]);
            }

            for(size_t i = 0; i < mPopulation.size() * pMutation; ++i)
            {
                uint32_t toMutate = randRangeExp(numElitists, mPopulation.size() - 1, 0.4);
                uint32_t mutated  = mPopulation.size() - 1 -
                                    randRangeExp(0, mPopulation.size() - 1, 0.4);
                mPopulation[toMutate] = mMutation(mBacking[mutated]);
            }

            std::sort(mPopulation.begin(),
                      mPopulation.end(),
                      comp);

            if(mListener)
            {
                mListener->onGeneration(gens+1, mFitness(mPopulation[0]));
            }
        }
        return mPopulation[0];
    }

private:
    /**
     * Returns a random number between 0.0 and 1.0.
     * Uses __rand()__ - seed with __srand()__ to be able to reproduce results.
     */
    FORCE_INLINE real_type randNum() const
    {
        return static_cast<float>(rand()) / RAND_MAX;
    }

    /**
      Calculates a random integer within the range of low and high (inclusive).
      Uniformly distributed.
      */
    FORCE_INLINE uint32_t randRange(uint32_t low, uint32_t high) const
    {
        return low + randNum() * (high - low);
    }

    /**
      Calculates a random integer within the range of low and high (inclusive).
      Exponentially distributed with a parameter mu.
      */
    uint32_t randRangeExp(uint32_t low, uint32_t high, real_type mu) const
    {
        const real_type lowExp = expf(-mu*low);
        const real_type val = (-1./mu) * logf( lowExp - randNum() * (lowExp - expf(-mu*high)) );
        const uint32_t retVal = floorf(val + 0.5);
        AI_ASSERT(retVal >= low && retVal <= high, "Must be in valid range.");
        return retVal;
    }

    std::vector<T> mPopulation, mBacking;
    FitnessFunction mFitness;
    CrossoverFunction mCrossover;
    MutationFunction mMutation;
    GeneratorFunction mGenerator;
};

END_NS_AILIB

#endif // GENETIC_H
