// Benchmark class that provides query benchmarking functionality for the PLY Gem.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <PLY/PLYResultBus.h>
#include <PLY/PLYTools.h>
#include <PLY/PLYTypes.h>
#include <PLYLog.h>

namespace PLY
{
	//Forward declarations.
	class PLYSystemComponent;

	class Benchmark :
		protected PLY::PLYResultBus::Handler
	{

	public:

		//Benchmarking modes.
		enum Mode { SIMPLE, STARS };

		Benchmark(PLYSystemComponent *psc, const Mode &m, const int &passes, const AZStd::string filenamePrefix);
		~Benchmark();

		//Run benchmark.
		void Run();

		//Stop current benchmark.
		void Stop();

		//Is the current benchmark finished?
		inline bool IsFinished() { return m_done; };

	private:

		//Pointer to PLYSystemComponent that owns the connections and queues.
		PLY::PLYSystemComponent *m_psc;

		//Benchmark mode.
		Mode m_mode;
		
		//Number of records to create when generating test data.
		int m_recordCount;

		//Chunks to break test data up into for creating multiple queries from test data.
		int m_chunks;

		//Start time of the benchmark test.
		AZ::ScriptTimePoint m_testStartTime;
		
		//End time of the benchmark test.
		AZ::ScriptTimePoint m_testEndTime;

		//Counts of rows of test data returned from the database for each query.
		std::vector<int> m_testDataRowCounts;

		//Query IDs associated with benchmark queries, so they benchmark query result sets can be identified.
		std::vector<unsigned long long> m_benchmarkQueryIDs;

		//Query ID for the "Vaccum Analyze" query.
		unsigned long long m_vacuumAnalyzeQueryID;

		//Query ID for the query that created the test data in the database.
		unsigned long long m_testDataGenerationQueryID;

		//ID of the current benchmark run.
		int m_runID;

		//Number of passes to perform per benchmark sequence.
		int m_passes;

		//Current pass in the benchmark seuqence.
		int m_curPass;

		//Prefix for benchmark results filenames.
		AZStd::string m_filenamePrefix;

		//Instruct the benchmark process to begin running.
		bool m_run;

		//Is the benchmark process running?
		bool m_running;

		//Is the benchmark process done?
		bool m_done;

		//Reset the benchmark run, in preparation for a new run.
		void Reset();

		//Advertises a result ID is ready.
		//@param queryID The ID of the ready result.
		void ResultReady(const unsigned long long queryID) override;

		//Save benchmark data to a file.
		//@param fileName The file name to save to.
		//@param append Should the data be appended to the file if it already exists?
		void SaveFileData(const AZStd::string fileName, const AZStd::string data, const bool append);

		//Start up the benchmark process.
		void Startup();

		//Shut down the benchmark process.
		void Shutdown();
	};
}