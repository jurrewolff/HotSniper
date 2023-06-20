/**
 * This header implements the DVFSPolicy interface.
 * A mapping policy is responsible for DVFS scaling.
 */

#ifndef __DVFSPOLICY_H
#define __DVFSPOLICY_H

#include <vector>
#include <unordered_map>
#include "performance_counters.h"

class DVFSPolicy {
public:
    virtual ~DVFSPolicy() {}
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores) = 0;

    std::unordered_map<int, int> coreAppIdMap;
};

#endif