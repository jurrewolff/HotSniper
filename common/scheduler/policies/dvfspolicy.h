/**
 * This header implements the DVFSPolicy interface.
 * A mapping policy is responsible for DVFS scaling.
 */

#ifndef __DVFSPOLICY_H
#define __DVFSPOLICY_H

#include <vector>
#include "performance_counters.h"

class DVFSPolicy {
public:
    virtual ~DVFSPolicy() {}
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores) = 0;
    virtual std::vector<int> getPriorities(const std::vector<std::tuple<int,int>> threadAppIdMapping) {
        return std::vector<int>(); // Return an empty vector as default
    }; // TODO - TESTING/DEMOING

};

#endif