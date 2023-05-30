/**
 * This header implements the max. freq DVFS policy
 */

#ifndef __DVFS_MAXFREQ_H
#define __DVFS_MAXFREQ_H

#include <vector>
#include <tuple>
#include "dvfspolicy.h"

class DVFSMaxFreq : public DVFSPolicy {
public:
    DVFSMaxFreq(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int maxFrequency);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores);
    virtual std::vector<int> getPriorities(const std::vector<std::tuple<int,int>> threadAppIdMapping); // TODO - TESTING/DEMOING

private:
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    int maxFrequency;
};

#endif