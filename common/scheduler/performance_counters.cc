#include "performance_counters.h"

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

vector<float> getLastNBeatValues(int appId, string targetColumn, unsigned int n) {
  vector<float> beatValues;

  string hbFilepath = to_string(appId) + ".hb.log";
  ifstream hbLogfile(hbFilepath);
  if (!hbLogfile.is_open()) {
    throw ios_base::failure("could not open hb logfile: " + hbFilepath);
  }

  string header;
  getline(hbLogfile, header);

  string line;
  vector<string> lines;
  while (getline(hbLogfile, line)) {
    lines.push_back(line);
  }

  if (lines.size() == 0) {
    throw out_of_range("no heartbeat data has been logged yet for appId " +
                       to_string(appId));
  }

  if (lines.size() >= n) {  // Only keep newest 'n' timestamps
    lines.erase(lines.begin(), lines.end() - n);
  }

  for (string& line : lines) {
    istringstream issHeader(header);
    istringstream issLine(line);
    string token;
    while (getline(issHeader, token, '\t')) {
      string value;
      getline(issLine, value, '\t');

      if (token == targetColumn) {
        beatValues.push_back(stof(value));
        break;
      }
    }

    if (beatValues.empty()) {
      throw runtime_error("column with name " + targetColumn +
                          " does not exist in hb file");
    }
  }

  return beatValues;
}

PerformanceCounters::PerformanceCounters(const char* output_dir, std::string instPowerFileNameParam, std::string instTemperatureFileNameParam, std::string instCPIStackFileNameParam)
    : outputDir(output_dir), instPowerFileName(instPowerFileNameParam), instTemperatureFileName(instTemperatureFileNameParam), instCPIStackFileName(instCPIStackFileNameParam) {

	//gkothar1: fix log file path names
	std::string temp = instPowerFileName;
	instPowerFileName = std::string(output_dir);
	instPowerFileName.append("/");
	instPowerFileName.append(temp);

	temp = instTemperatureFileName;
	instTemperatureFileName = std::string(output_dir);
	instTemperatureFileName.append("/");
	instTemperatureFileName.append(temp);

	temp = instCPIStackFileName;
	instCPIStackFileName = std::string(output_dir);
	instCPIStackFileName.append("/");
	instCPIStackFileName.append(temp);
}

/** getPowerOfComponent
    Returns the latest power consumption of a component being tracked using base.cfg. Return -1 if power value not found.
*/
double PerformanceCounters::getPowerOfComponent (string component) const {
	ifstream powerLogFile(instPowerFileName);
	string header;
	string footer;

	if (powerLogFile.good()) {
		getline(powerLogFile, header);
		getline(powerLogFile, footer);
	}

	std::istringstream issHeader(header);
	std::istringstream issFooter(footer);
	std::string token;

	while(getline(issHeader, token, '\t')) {
		std::string value;
		getline(issFooter, value, '\t');
		if (token == component) {
			return stod (value);
		}
	}

	return -1;
}

/** getPowerOfCore
 * Return the latest total power consumption of the given core. Requires "tp" (total power) to be tracked in base.cfg. Return -1 if power is not tracked.
 */
double PerformanceCounters::getPowerOfCore(int coreId) const {
	string component = "Core" + std::to_string(coreId) + "-TP";
	return getPowerOfComponent(component);
}


/** getPeakTemperature
    Returns the latest peak temperature of any component
*/
double PerformanceCounters::getPeakTemperature () const {
	ifstream temperatureLogFile(instTemperatureFileName);
	string header;
	string footer;

	if (temperatureLogFile.good()) {
		getline(temperatureLogFile, header);
		getline(temperatureLogFile, footer);
	}

	std::istringstream issFooter(footer);

	double maxTemp = -1;
	std::string value;
	while(getline(issFooter, value, '\t')) {
		double t = stod (value);
		if (t > maxTemp) {
			maxTemp = t;
		}
	}

	return maxTemp;
}


/** getTemperatureOfComponent
    Returns the latest temperature of a component being tracked using base.cfg. Return -1 if power value not found.
*/
double PerformanceCounters::getTemperatureOfComponent (string component) const {
	ifstream temperatureLogFile(instTemperatureFileName);
	string header;
	string footer;

  	if (temperatureLogFile.good()) {
		getline(temperatureLogFile, header);
		getline(temperatureLogFile, footer);
  	}

	std::istringstream issHeader(header);
	std::istringstream issFooter(footer);
	std::string token;

	while(getline(issHeader, token, '\t')) {
		std::string value;
		getline(issFooter, value, '\t');

		if (token == component) {
			return stod (value);
		}
	}

	return -1;
}

/** getTemperatureOfCore
 * Return the latest temperature of the given core. Requires "tp" (total power) to be tracked in base.cfg. Return -1 if power is not tracked.
 */
double PerformanceCounters::getTemperatureOfCore(int coreId) const {
	string component = "Core" + std::to_string(coreId) + "-TP";
	return getTemperatureOfComponent(component);
}

/**
 * Get a performance metric for the given core.
 * Available performance metrics can be checked in InstantaneousPerformanceCounters.log
 */
double PerformanceCounters::getCPIStackPartOfCore(int coreId, std::string metric) const {
	ifstream cpiStackLogFile(instCPIStackFileName);
    string line;
	std::istringstream issLine;

	// first find the line in the logfile that contains the desired metric
	bool metricFound = false;
	while (!metricFound) {
  		if (cpiStackLogFile.good()) {
			getline(cpiStackLogFile, line);
			issLine.str(line);
			issLine.clear();
			std::string m;
			getline(issLine, m, '\t');
			metricFound = (m == metric);
		} else {
			return -1;
		}
	}
	
	// then split the coreId-th value from this line (first value is metric name, but already consumed above)
	std::string value;
	for (int i = 0; i < coreId + 1; i++) {
		getline(issLine, value, '\t');
		if ((i == 0) && (value == "-")) {
			return 0;
		}
	}

	return stod(value);
}

/**
 * Get the utilization of the given core.
 */
double PerformanceCounters::getUtilizationOfCore(int coreId) const {
	return getCPIStackPartOfCore(coreId, "base") / getCPIOfCore(coreId);
}

/**
 * Get the CPI of the given core.
 */
double PerformanceCounters::getCPIOfCore(int coreId) const {
	return getCPIStackPartOfCore(coreId, "total");
}

/**
 * Get the rel. NUCA part of the CPI stack of the given core.
 */
double PerformanceCounters::getRelNUCACPIOfCore(int coreId) const {
	return getCPIStackPartOfCore(coreId, "mem-nuca") / getCPIOfCore(coreId);
}

/**
 * Get the frequency of the given core.
 */
int PerformanceCounters::getFreqOfCore(int coreId) const {
	if (coreId >= (int)frequencies.size()) {
		return -1;
	} else {
		return frequencies.at(coreId);
	}
}

/**
 * Notify new frequencies
 */
void PerformanceCounters::notifyFreqsOfCores(std::vector<int> newFrequencies) {
	frequencies = newFrequencies;
}

/**
 * Get the frequency of the given core.
 */
double PerformanceCounters::getIPSOfCore(int coreId) const {
	return 1e6 * getFreqOfCore(coreId) / getCPIOfCore(coreId);
}

/**
 * Get the minimum heartrate for application with appId.
 */
float PerformanceCounters::getMinHeartrate(int appId) const {
	vector<float> beatValues;

	try {
		beatValues = getLastNBeatValues(appId, "Min Rate", 1);
	} catch(const std::exception& e) {
		cout << "[PerformanceCounters] error getting beat values: " << e.what() << endl;
		return -1.0;
	}

	if (beatValues.empty()) {
		cout << "[PerformanceCounters] no heartbeat data found" << endl;
		return -1.0;
	}

	return beatValues[0];
}

/**
 * Get the maximum heartrate for application with appId.
 */
float PerformanceCounters::getMaxHeartrate(int appId) const {
	vector<float> beatValues;

	try {
		beatValues = getLastNBeatValues(appId, "Max Rate", 1);
	} catch(const std::exception& e) {
		cout << "[PerformanceCounters] error getting beat values: " << e.what() << endl;
		return -1.0;
	}

	if (beatValues.empty()) {
		cout << "[PerformanceCounters] no heartbeat data found" << endl;
		return -1.0;
	}

	return beatValues[0];
}

/**
 * Get the last registered heartbeat timestamp.
 */
int PerformanceCounters::getLastBeat(int appId) const {
	vector<float> beatValues;

	try {
		beatValues = getLastNBeatValues(appId, "Timestamp", 1);
	} catch(const std::exception& e) {
		cout << "[PerformanceCounters] error getting beat values: " << e.what() << endl;
		return -1;
	}

	if (beatValues.empty()) {
		cout << "[PerformanceCounters] no heartbeat data found" << endl;
		return -1;
	}

	return (int)beatValues[0];
}

/**
 * Get the last registered heartbeat instant rate value (beats per second, over
 * last two beats)
 */
float PerformanceCounters::getLastInstantRate(int appId) const {
	vector<float> beatValues;

	try {
		beatValues = getLastNBeatValues(appId, "Instant Rate", 1);
	} catch (const std::exception& e) {
		cout << "[PerformanceCounters] error getting beat values: " << e.what() << endl;
		return -1.0;
	}

	if (beatValues.empty()) {
		cout << "[PerformanceCounters] no heartbeat data found" << endl;
		return -1.0;
	}

	return beatValues[0];
}

/**
 * Get heartbeat timestamp history of 'n' number of records.
 */
vector<float> PerformanceCounters::getBeatHistory(int appId, unsigned int n) const {
	vector<float> beatValues{};

	try {
		beatValues = getLastNBeatValues(appId, "Instant Rate", n);
	} catch (const std::exception& e) {
		cout << "[PerformanceCounters] error getting beat values: " << e.what() << endl;
		return beatValues;
	}

	if (beatValues.empty()) {
		cout << "[PerformanceCounters] no heartbeat data found" << endl;
	}

	return beatValues;
}
