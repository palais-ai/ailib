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
            return getFitnessForValue(lv) < getFitnessForValue(rv); // Sort from best to worst.
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
        for(size_t gens = 0; gens < numGenerations; ++gens)
        {
            // A full sort is not necessary, we could just pick out a few and take the best of those
            std::sort(mPopulation.begin(),
                      mPopulation.end(),
                      comp);

            if(mListener)
            {
                mListener->onGeneration(gens+1, mFitness(mPopulation[0]));
            }

            const uint32_t numElitists = pElitism * mPopulation.size();
            for(size_t i = 0; i < mPopulation.size() * pCrossover; ++i)
            {
                uint32_t first = randRange(numElitists, mPopulation.size());
                uint32_t second = 0;
                do
                {
                    second = randRange(numElitists, mPopulation.size());
                } while(first == second);

                mPopulation[first] = mCrossover(mPopulation[first], mPopulation[second]);
            }

            for(size_t i = 0; i < mPopulation.size() * pMutation; ++i)
            {
                uint32_t toMutate = randRange(numElitists, mPopulation.size());
                mPopulation[toMutate] = mMutation(mPopulation[toMutate]);
            }
        }
        return mPopulation[0];
    }

private:
    FORCE_INLINE uint32_t randRange(uint32_t low, uint32_t high)
    {
        return low + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (high - low));
    }

    std::vector<T> mPopulation;
    FitnessFunction mFitness;
    CrossoverFunction mCrossover;
    MutationFunction mMutation;
    GeneratorFunction mGenerator;
};

END_NS_AILIB

#endif // GENETIC_H
