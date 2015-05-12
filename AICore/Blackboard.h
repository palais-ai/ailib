#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#pragma once

#include "ai_global.h"
#include "Any.h"
#include "btHashMap.h"
#include <map>

BEGIN_NS_AILIB

template <typename KEY>
class BlackboardListener
{
public:
    virtual void onValueChanged(const KEY& key, const ailib::hold_any& value) = 0;
};


typedef uint32_t Handle;
const static Handle INVALID_HANDLE = 0;

/**
 * @brief Blackboard A generic class to store knowledge associated to unique keys.
 */
template <typename KEY>
class Blackboard
{
    typedef std::map<Handle, BlackboardListener<KEY>* > BlackboardListeners;
public:
    typedef KEY key_type;

    Blackboard() :
        mCount(INVALID_HANDLE)
    {
        ;
    }

    // INVALID_HANLDE (= 0) is never returned.
    Handle addListener(BlackboardListener<KEY>* listener)
    {
        AI_ASSERT(listener, "Tried to add NULL listener.");
        mListeners.insert(std::make_pair(++mCount, listener));
        return mCount;
    }

    void removeListener(Handle id)
    {
        size_t numRemoved = mListeners.erase(id);
        AI_ASSERT(numRemoved == 1, "Tried to remove non-existant listener.");
    }

    template <typename T>
    FORCE_INLINE T get(const KEY& key) const
    {
        return ailib::any_cast<T>(mKnowledge[key]);
    }

    FORCE_INLINE bool has(const KEY& key) const
    {
        return mKnowledge.find(key) != NULL;
    }

    template <typename T>
    void set(const KEY& key, const T& value)
    {
        ailib::hold_any newVal(value);
        mKnowledge.insert(key, newVal);

        for(typename BlackboardListeners::iterator it = mListeners.begin();
            it != mListeners.end(); ++it)
        {
            it->second->onValueChanged(key, newVal);
        }
    }

    FORCE_INLINE void remove(const KEY& key)
    {
        mKnowledge.remove(key);
    }

    uint32_t getHashCode() const
    {
        // CREDITS: FNV-1a algorithm primes from http://www.isthe.com/chongo/tech/comp/fnv/
        // (public domain)
        static const uint32_t fnv_prime    =   16777619;
        static const uint32_t offset_basis = 2166136261;

        uint32_t hash = offset_basis;

        // Combine the hashes of the individual keys.
        for(int i = 0; i < mKnowledge.size(); ++i)
        {
            hash *= fnv_prime;
            hash += mKnowledge.getKeyAtIndex(i).getHash();
        }
    }

    FORCE_INLINE int size() const
    {
        return mKnowledge.size();
    }

    bool operator==(const Blackboard& other) const
    {
        if(size() != other.size())
        {
            return false;
        }

        for(int i = 0; i < mKnowledge.size(); ++i)
        {
            const KEY& key = mKnowledge.getKeyAtIndex(i);
            const ailib::hold_any* value = other.mKnowledge.find(key);

            if(value == NULL ||
               *value != *mKnowledge.getAtIndex(i))
            {
                return false;
            }
        }

        return true;
    }

private:
    btHashMap<KEY, ailib::hold_any> mKnowledge;
    BlackboardListeners mListeners;
    uint32_t mCount;
};

// Adapter class to enable the use of Blackboards as keys in btHashMap.
template <typename KEY>
class HashBlackboard
{
private:
    Blackboard<KEY>*  mBlackboard;
    unsigned int mHash;
public:
    HashBlackboard(Blackboard<KEY>* board) :
        mBlackboard(board),
        mHash(board->getHashCode())
    {
        ;
    }

    FORCE_INLINE unsigned int getHash() const
    {
        return mHash;
    }

    FORCE_INLINE bool equals(const HashBlackboard& other) const
    {
        return mBlackboard == other.mBlackboard;
    }
};

END_NS_AILIB

#endif // BLACKBOARD_H
