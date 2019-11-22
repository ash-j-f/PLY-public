// Statistics collector for the PLY Gem. Designed to be used as a singleton via the provided macro.
// Collects statistics about the query worker threads, and the queries they are processing.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <atomic>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Component/TickBus.h>

#define STATS PLY::StatsCollector::getInstance()

namespace PLY
{
	class StatsCollector
	{
	public:
		
		//Get a singleton instance.
		static StatsCollector *getInstance();

		//Start interval display of query stats in the game console.
		inline void Start() { m_showStats = true; };

		//Stop interval display of query stats in the game console.
		inline void Stop() { m_showStats = false; };

		//Set the time interval between display of query stats in the game console.
		inline void SetInterval(int i) { AZ_Error("PLY", i > 0, "Stats interval must be greater than 0."); m_interval = i; };

		//Increment the query counter.
		inline void CountQuery() { if (m_showStats) m_querySentCount++; };

		//Increment the results counter.
		inline void CountResult() { if (m_showStats) m_resultsReceivedCount++; };

		//Tick handler.
		void OnTick(float deltaTime, AZ::ScriptTimePoint time);

		//Get the maxiumum number of busy worker threads reached during program execution.
		inline int GetMaxBusyThreadsOverall() const { return m_maxBusyWorkersOverallStat; };

		//Adjust the busy worker threads statistic by a set amount.
		//@param change The amount to change the busy worker threads stat by.
		void AdjustBusyWorkersOverallStat(int change);

		//Reset the busy worker threads stat to zero.
		void ResetBusyWorkersOverallStat();

	private:

		//Interval between display of statistics in the console (in seconds).
		int m_interval;

		//Time since statistics were last printed to the console.
		float m_timer;

		//Should stats be printed to the console each time interval?
		bool m_showStats;

		//Number of queries sent since the last interval start.
		int m_querySentCount;

		//Number of results received since the last interval start.
		int m_resultsReceivedCount;
		
		////
		//The following properties use ATOMIC to make adjusting and reading these stats thread-safe.
		////

		//Statistic for max number of workers that were working on queries simultaneously, since program start.
		std::atomic<int> m_maxBusyWorkersOverallStat;

		//Current number of workers that were working on queries simultaneously, since program start.
		std::atomic<int> m_busyWorkersOverallStat;

		//Statistic for max number of workers that were working on queries simultaneously, since last interval start.
		std::atomic<int> m_maxBusyWorkersStat;

		//Current number of workers that were working on queries simultaneously, since last interval start.
		std::atomic<int> m_busyWorkersStat;

		//Reset all interval statistics to zero.
		void ResetStats();

		StatsCollector();
		~StatsCollector();
	};
}