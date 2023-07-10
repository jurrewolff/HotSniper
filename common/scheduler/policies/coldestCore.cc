#include "coldestCore.h"
#include <iomanip>
using namespace std;

float tempThreshold = 65.0f; // For this demo hardcoded global is fine.

ColdestCore::ColdestCore(
    const PerformanceCounters *performanceCounters,
    int coreRows,
    int coreColumns,
    float criticalTemperature)
    : performanceCounters(performanceCounters),
      coreRows(coreRows),
      coreColumns(coreColumns),
      criticalTemperature(criticalTemperature)
{
}
std::vector<int> ColdestCore::map(
    String taskName,
    int taskCoreRequirement,
    const std::vector<bool> &availableCoresRO,
    const std::vector<bool> &activeCores)
{
  std::vector<bool> availableCores(availableCoresRO);
  std::vector<int> cores;
  logTemperatures(availableCores);
  for (; taskCoreRequirement > 0; taskCoreRequirement--)
  {
    int coldestCore = getColdestCore(availableCores);
    if (coldestCore == -1)
    {
      // not enough free cores
      std::vector<int> empty;
      return empty;
    }
    else
    {
      cores.push_back(coldestCore);
      availableCores.at(coldestCore) = false;
    }
  }
  return cores;
}
std::vector<migration> ColdestCore::migrate(
    SubsecondTime time,
    const std::vector<int> &taskIds,
    const std::vector<bool> &activeCores)
{
  std::vector<migration> migrations;
  std ::vector<bool> availableCores(coreRows * coreColumns);
  for (int c = 0; c < coreRows * coreColumns; c++)
  {
    availableCores.at(c) = taskIds.at(c) == -1;
  }
  for (int c = 0; c < coreRows * coreColumns; c++)
  {
    if (activeCores.at(c))
    {
      float curCoreTemp = performanceCounters->getTemperatureOfCore(c);
      float prevCoreTemp = c > 0 ? performanceCounters->getTemperatureOfCore(c-1) : -1.0f;
      float nextCoreTemp = c < (coreRows * coreColumns - 1) ? performanceCounters->getTemperatureOfCore(c+1) : -1.0f;
      if (curCoreTemp > tempThreshold && prevCoreTemp < tempThreshold && nextCoreTemp < tempThreshold)
      { // Migrate when hot core is surrounded by cold cores.
        cout << "[Scheduler][coldestCore-migrate]: core" << c << " is hot (";
        cout << fixed << setprecision(1) << curCoreTemp << ") and surrounded by cold cores " << endl;
        logTemperatures(availableCores);
        int targetCore = getCoreNextToHotCore(availableCores, c);
        if (targetCore == -1)
        {
          cout << "[Scheduler][coldestCore-migrate]: no target core found, cannot migrate " << endl;
        }
        else
        {
          migration m;
          m.fromCore = c;
          m.toCore = targetCore;
          m.swap = false;
          migrations.push_back(m);
          availableCores.at(targetCore) = false;
          break; // Only move one core. Helps keep simplicity of code. Ok for demo.
        }
      }
    }
  }
  return migrations;
}
int ColdestCore::getColdestCore(const std::vector<bool> &availableCores)
{
  int coldestCore = -1;
  float coldestTemperature = 0;
  // iterate all cores to find coldest
  for (int c = 0; c < coreRows * coreColumns; c++)
  {
    if (availableCores.at(c))
    {
      float temperature = performanceCounters->getTemperatureOfCore(c);
      if ((coldestCore == -1) || (temperature < coldestTemperature))
      {
        coldestCore = c;
        coldestTemperature = temperature;
      }
    }
  }
  return coldestCore;
}
int ColdestCore::getCoreNextToHotCore(const std::vector<bool> &availableCores, int except) {
  for (int c = 0; c < coreRows * coreColumns; c++)
  {
    if (availableCores.at(c))
    {
      // TODO - Check core before and after, if any of them are hot, return its
      //        index.
      float prevCoreTemp = c > 0 ? performanceCounters->getTemperatureOfCore(c-1) : -1.0f;
      float nextCoreTemp = c < (coreRows * coreColumns - 1) ? performanceCounters->getTemperatureOfCore(c+1) : -1.0f;
    
      if (c > 0 && c-1 != except && !availableCores.at(c-1) && prevCoreTemp > tempThreshold)
      { // Prev core exists, is not excepted, is in use and hot 
        return c;
      } else if (c < (coreRows * coreColumns - 1) && c+1 != except && !availableCores.at(c+1) && nextCoreTemp > tempThreshold)
      {
        return c;
      }
    }
  }

  return -1;
}
void ColdestCore::logTemperatures(const std::vector<bool> &availableCores)
{
  cout << "[Scheduler][coldestCore-map]: temperatures of available cores:" << endl;
  for (int y = 0; y < coreRows; y++)
  {
    for (int x = 0; x < coreColumns; x++)
    {
      if (x > 0)
      {
        cout << " ";
      }
      int coreId = y * coreColumns + x;
      if (!availableCores.at(coreId))
      {
        cout << " - ";
      }
      else
      {
        float temperature = performanceCounters->getTemperatureOfCore(
            coreId);
        cout << fixed << setprecision(1) << temperature;
      }
    }
    cout << endl;
  }
}