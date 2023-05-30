#include "dvfsMaxFreq.h"
#include <iomanip>
#include <iostream>
#include <algorithm>

using namespace std;

DVFSMaxFreq::DVFSMaxFreq(const PerformanceCounters *performanceCounters, int coreRows, int coreColumns, int maxFrequency)
	: performanceCounters(performanceCounters), coreRows(coreRows), coreColumns(coreColumns), maxFrequency(maxFrequency) {
	
}

std::vector<int> DVFSMaxFreq::getFrequencies(const std::vector<int> &oldFrequencies, const std::vector<bool> &activeCores) {
	std::vector<int> frequencies(coreRows * coreColumns);

	for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns; coreCounter++) {
		if (activeCores.at(coreCounter)) {
			float power = performanceCounters->getPowerOfCore(coreCounter);
			float temperature = performanceCounters->getTemperatureOfCore(coreCounter);
			int frequency = oldFrequencies.at(coreCounter);
			float utilization = performanceCounters->getUtilizationOfCore(coreCounter);

			cout << "[Scheduler][DVFS_MAX_FREQ]: Core " << setw(2) << coreCounter << ":";
			cout << " P=" << fixed << setprecision(3) << power << " W";
			cout << " f=" << frequency << " MHz";
			cout << " T=" << fixed << setprecision(1) << temperature << " °C";
			cout << " utilization=" << fixed << setprecision(3) << utilization << endl;
		}
		frequencies.at(coreCounter) = maxFrequency;
	}

	return frequencies;
}

// TODO - This is a demo method that shows how a heartbeat policy could work.
//				Instead of creating an entire new hb related policy class I've just
//				added it to DVFSMaxFreq for demonstration purposes.
// 			- Also, the function can probably be optimized, for instance using pairs
//				and other readability changes.
std::vector<int> DVFSMaxFreq::getPriorities(const std::vector<std::tuple<int,int>> threadAppIdMapping) {

	// The algorithm iterates over the vector containing tuples consisting of
	// thread id and app id and gets the last heartbeat for each app id. The
	// thread ids and their corresponding last beats are stored in a vector of 
	// tuples again, which is then sorted on the smallest last beat timestamp.
	// Finally the thread ids of the sorted vector are extracted into another
	// vector and this vector of thread ids is returned.

	std::vector<std::tuple<int,int>> threadLastBeats;
	for (size_t t = 0; t < threadAppIdMapping.size(); t++) {
		std::tuple<int,int> tidAppIdPair = threadAppIdMapping[t];
		int lastBeat = performanceCounters->getLastBeat(std::get<1>(tidAppIdPair));

		std::tuple<int,int> threadlastBeatPair = std::make_tuple(std::get<0>(tidAppIdPair), lastBeat);
		threadLastBeats.push_back(threadlastBeatPair);
	}

	std::sort(threadLastBeats.begin(), threadLastBeats.end(), [](const std::tuple<int,int> t1, const std::tuple<int,int> t2) {return std::get<1>(t1) < std::get<1>(t2);});

	std::vector<int> priorities;
	for (std::tuple<int,int> t : threadLastBeats) {
		cout << "[DVFSMAXFREQ.getPriorities][TESTING] Last beat for TID " << std::get<0>(t) << " is: " << std::get<1>(t) << endl;
		priorities.push_back(std::get<0>(t));
	}

	return priorities;
}
