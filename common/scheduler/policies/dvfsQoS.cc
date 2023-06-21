#include "dvfsQoS.h"

#include <iomanip>
#include <iostream>

using namespace std;

DVFSQoS::DVFSQoS(const PerformanceCounters *performanceCounters, int coreRows,
                 int coreColumns, int maxFrequency, int minFrequency,
                 float margin, int step_size)
    : performanceCounters(performanceCounters),
      coreRows(coreRows),
      coreColumns(coreColumns),
      maxFrequency(maxFrequency),
      minFrequency(minFrequency),
      margin(margin),
      step_size(step_size) {
  for (int i = 0; i < coreRows * coreColumns; i++) {
    corePrevQosMapping[i] = -1.0f;
  }
}

vector<int> DVFSQoS::getFrequencies(const vector<int> &oldFrequencies,
                                    const vector<bool> &activeCores) {
  vector<int> frequencies(coreRows * coreColumns);

  for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns;
       coreCounter++) {
    if (activeCores.at(coreCounter)) {
      float power = performanceCounters->getPowerOfCore(coreCounter);
      float temperature =
          performanceCounters->getTemperatureOfCore(coreCounter);
      int frequency = oldFrequencies.at(coreCounter);
      float utilization =
          performanceCounters->getUtilizationOfCore(coreCounter);

      cout << "[Scheduler][DVFS_QOS]: Core " << setw(2) << coreCounter << ":";
      cout << " P=" << fixed << setprecision(3) << power << " W";
      cout << " f=" << frequency << " MHz";
      cout << " T=" << fixed << setprecision(1) << temperature << " °C";
      cout << " utilization=" << fixed << setprecision(3) << utilization
           << endl;
    }

    int appId = -1;
    try {
      appId = coreAppIdMap.at(coreCounter);
    } catch (out_of_range) {
      // Core has no thread assigned to it, set minimum freq
      frequencies.at(coreCounter) = minFrequency;
      continue;
    }

    const float currentQos = performanceCounters->getLastInstantRate(appId);
    if (currentQos < 0.0f) {
      // Core has no heartbeat data yet
      frequencies.at(coreCounter) = maxFrequency;
      continue;
    }

    if (currentQos == corePrevQosMapping[coreCounter]) {
      // hb data not updated, keep old freq
      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter);
      continue;
    } else {
      corePrevQosMapping[coreCounter] = currentQos;
    }

    float targetQoS = performanceCounters->getMinHeartrate(appId);
    if (abs(targetQoS) < 0.00001) {  // Check for approximate 0
      // Unconfigured min qos value for app. Set max freq.
      frequencies.at(coreCounter) = maxFrequency;
      continue;
    }

    if (targetQoS <= 0.0f) {
      // Error getting the target qos value. Keep old freq.
      cout << "[Scheduler][DVFS_QOS] error getting the target QoS for app ID "
           << appId << " running on core " << coreCounter
           << " -> keeping previous frequency" << endl;

      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter);
      continue;
    }

    const float lowerBound = targetQoS - (margin / 2.0f);
    const float upperBound = targetQoS + (margin / 2.0f);
    if (currentQos >= lowerBound && currentQos <= upperBound) {
      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter);
    } else if (currentQos > targetQoS) {
      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter) - step_size;
    } else {
      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter) + step_size;
    }

    cout << "[Scheduler][DVFS_QOS][DEBUG] Set freq of core " << coreCounter
         << " to " << frequencies.at(coreCounter) << endl;
  }

  return frequencies;
}