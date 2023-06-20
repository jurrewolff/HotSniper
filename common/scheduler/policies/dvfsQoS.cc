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
      cout << "[Scheduler][DVFS_QOS][DEBUG] Core " << coreCounter
           << " has no assigned thread -> Setting min freq" << endl;

      frequencies.at(coreCounter) = minFrequency;
      continue;
    }

    const float currentQos = performanceCounters->getLastInstantRate(appId);
    if (currentQos < 0.0f) {
      cout << "[Scheduler][DVFS_QOS][DEBUG] Core " << coreCounter
           << " running appId " << appId
           << " has no hb data yet -> Seting max freq" << endl;

      frequencies.at(coreCounter) = maxFrequency;
      continue;
    }

    if (currentQos == corePrevQosMapping[coreCounter]) {
      cout << "[Scheduler][DVFS_QOS][DEBUG] Core " << coreCounter
           << " running appId " << appId
           << " its hb data has not updated -> Keeping old freq" << endl;

      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter);
      continue;
    } else {
      corePrevQosMapping[coreCounter] = currentQos;
    }

    float qos = performanceCounters->getMinHeartrate(appId);
    if (qos <= 0.0f) {  // -1.0 is error, 0.0 is unconfigured min val for app.
      cout << "[Scheduler][DVFS_QOS][DEBUG] Could not get min heartrate for "
              "core "
           << coreCounter << " running appId " << appId << "-> Keeping old freq"
           << endl;

      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter);
      continue;
    }

    const float lowerBound = qos - (margin / 2.0f);
    const float upperBound = qos + (margin / 2.0f);
    if (currentQos >= lowerBound && currentQos <= upperBound) {
      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter);
    } else if (currentQos > qos) {
      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter) - step_size;
    } else {
      frequencies.at(coreCounter) = oldFrequencies.at(coreCounter) + step_size;
    }

    cout << "[Scheduler][DVFS_QOS][DEBUG] Set freq of core " << coreCounter
         << " to " << frequencies.at(coreCounter) << endl;
  }

  return frequencies;
}