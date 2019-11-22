// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include "StatsCollector.h"
#include <string>

using namespace PLY;

void PLY::StatsCollector::ResetStats()
{
	m_querySentCount = 0;
	m_resultsReceivedCount = 0;

	int m_busyWorkersStatTEMP = m_busyWorkersStat;
	m_maxBusyWorkersStat = m_busyWorkersStatTEMP;
}

StatsCollector::StatsCollector()
	: m_interval(5), //Default stats display interval.
	m_timer(0),
	m_showStats(false),
	m_querySentCount(0),
	m_resultsReceivedCount(0),
	m_maxBusyWorkersOverallStat(0),
	m_busyWorkersOverallStat(0),
	m_maxBusyWorkersStat(0),
	m_busyWorkersStat(0)
{
	
}

StatsCollector::~StatsCollector()
{

}

StatsCollector *StatsCollector::getInstance()
{
	static StatsCollector instance;
	return &instance;
}

void PLY::StatsCollector::AdjustBusyWorkersOverallStat(int change)
{
	m_busyWorkersOverallStat += change;
	m_busyWorkersStat += change;

	//Temp copy as atomic can't be directly copied to another atomic.
	int m_busyWorkersOverallStatTEMP = m_busyWorkersOverallStat;
	if (m_maxBusyWorkersOverallStat < m_busyWorkersOverallStat) m_maxBusyWorkersOverallStat = m_busyWorkersOverallStatTEMP;

	int m_busyWorkersStatTEMP = m_busyWorkersStat;
	if (m_maxBusyWorkersStat < m_busyWorkersStat) m_maxBusyWorkersStat = m_busyWorkersStatTEMP;
}

void PLY::StatsCollector::ResetBusyWorkersOverallStat()
{
	m_maxBusyWorkersOverallStat = 0;
	m_busyWorkersOverallStat = 0;
}

void PLY::StatsCollector::OnTick(float deltaTime, AZ::ScriptTimePoint time)
{
	if (m_showStats)
	{
		m_timer += deltaTime;
		if (m_timer >= m_interval)
		{

			float qSentPerSec = m_querySentCount > 0 ? (float)m_querySentCount / m_timer : 0;
			float qResultsPerSec = m_resultsReceivedCount > 0 ? (float)m_resultsReceivedCount / m_timer : 0;

			std::string outstr = "PLY STATS: " + std::to_string(qSentPerSec) + " queries sent/sec. "
				+ std::to_string(qResultsPerSec) + " results received/sec. "
				+ std::to_string(m_maxBusyWorkersStat) + " max busy workers.";

			AZ_Printf("PLY", "%s", outstr.c_str());

			m_timer = 0;
			ResetStats();
		}
	}
}
