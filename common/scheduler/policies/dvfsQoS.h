/**
 * This header implements Quality of Service DVFS policy
 */

#ifndef __DVFS_QOS_H
#define __DVFS_QOS_H

#include <vector>

#include "dvfspolicy.h"

class DVFSQoS : public DVFSPolicy {
 public:
  DVFSQoS(const PerformanceCounters *performanceCounters, int coreRows,
          int coreColumns, int maxFrequency, int minFrequency, float margin, int step_size);
  virtual std::vector<int> getFrequencies(
      const std::vector<int> &oldFrequencies,
      const std::vector<bool> &activeCores);

 private:
  const PerformanceCounters *performanceCounters;
  unsigned int coreRows;
  unsigned int coreColumns;
  int maxFrequency;
  int minFrequency;
  float margin;
  int step_size;

  std::unordered_map<int, float> corePrevQosMapping;
};

#endif