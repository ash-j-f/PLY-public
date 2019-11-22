// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include "Benchmark.h"

#include <AzCore/IO/FileIO.h>

#include <PLY/PLYRequestBus.h>
#include <PLYSystemComponent.h>
#include <StatsCollector.h>

using namespace PLY;

Benchmark::Benchmark(PLYSystemComponent *psc, const Mode &m, const int &passes, const AZStd::string filenamePrefix)
	: m_psc(psc),
	m_recordCount(0),
	m_vacuumAnalyzeQueryID(0),
	m_testDataGenerationQueryID(0),
	m_run(false),
	m_running(false),
	m_done(false),
	m_mode(m),
	m_passes(passes),
	m_curPass(0),
	m_filenamePrefix(filenamePrefix)
{
	AZ_Error("PLY", passes > 0, "Passes count must be greater than 0");

	//Set Run ID to an integer based on current time in millseconds.
	AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
	m_runID = static_cast<int>(AZ::ScriptTimePoint(now).GetMilliseconds());

	Startup();
}

Benchmark::~Benchmark()
{
	Shutdown();
}

void Benchmark::Run()
{
	if (m_running)
	{
		AZ_Printf("Script", "%s", "Benchmark already running.");
		return;
	}

	m_run = true;
	m_running = true;

	if (m_mode == SIMPLE)
	{
		AZ_Printf("Script", "%s", "Starting SIMPLE DATASET Benchmark...");
	}
	else if (m_mode == STARS)
	{
		AZ_Printf("Script", "%s", ("Starting STARS DATASET Benchmark... Run " + std::to_string(m_curPass + 1) +  " of " + std::to_string(m_passes) + ".").c_str());
	}
	else
	{
		PLYLOG(PLY::PLYLog::PLY_ERROR, "Unknown benchmark mode. Stopping benchmark.");
		Stop();
	}

	m_psc->InitialisePool();

	STATS->ResetBusyWorkersOverallStat();

	unsigned long long queryID = 0;
	AZStd::string qString = "";

	//Query settings. Override all defaults.
	QuerySettings qs;
	qs.queryTTL = 600000;
	qs.resultTTL = 600000;
	qs.advertiseResult = true;
	qs.useTransaction = true;

	if (m_mode == SIMPLE)
	{
		//Create test data.

		PLYLOG(PLY::PLYLog::PLY_INFO, "Creating benchmark test data.");

		//Records to return IN TOTAL.
		m_recordCount = 1000000;

		qString = ("DO\
			$do$\
			BEGIN\
			IF NOT EXISTS(SELECT 1 FROM information_schema.tables WHERE table_schema = 'public' AND table_name = 'ply_test_data') THEN\
				CREATE TABLE ply_test_data(id integer PRIMARY KEY, rnd double precision);\
				INSERT INTO ply_test_data(id, rnd)\
					SELECT x.id, random()\
					FROM generate_series(1, " + std::to_string(m_recordCount) + ") AS x(id) ON CONFLICT DO NOTHING;\
			END IF;\
			END\
			$do$;").c_str();
	}
	else if (m_mode == STARS)
	{
		//Check for star data.

		PLYLOG(PLY::PLYLog::PLY_INFO, "Checking for sample star data.");

		qString = "SELECT EXISTS(\
			SELECT 1\
			FROM   information_schema.tables\
			WHERE  table_schema = 'public'\
			AND    table_name = 'gaia_main'\
			);";

	}

	PLY::PLYRequestBus::BroadcastResult(queryID, &PLY::PLYRequestBus::Events::SendQueryWithOptions, qString, qs);
	if (queryID == 0)
	{
		if (m_mode == SIMPLE)
		{
			PLYLOG(PLY::PLYLog::PLY_ERROR, "Benchmark failed. Couldn't send create test data set query. Stopping.");
		}
		else if (m_mode == STARS)
		{
			PLYLOG(PLY::PLYLog::PLY_ERROR, "Benchmark failed. Couldn't send check for star data set query. Stopping.");
		}
		Stop();
		return;
	}

	m_testDataGenerationQueryID = queryID;

}

void PLY::Benchmark::Stop()
{
	if (m_running)
	{
		m_run = false;
		m_running = false;
		AZ_Printf("PLY", "%s", "Stopping benchmark.");
	}
	else
	{
		AZ_Printf("PLY", "%s", "Benchmark is not running.");
	}
}

void PLY::Benchmark::Reset()
{
}

void Benchmark::ResultReady(const unsigned long long queryID)
{

	if (!m_run) return;

	//Query settings. Override all defaults.
	QuerySettings qs;
	qs.queryTTL = 600000;
	qs.resultTTL = 600000;
	qs.advertiseResult = true;
	qs.useTransaction = true;

	//Is this the test data generation query result?
	//Then run vacuum analyze.
	if (queryID == m_testDataGenerationQueryID)
	{

		unsigned long long nextQueryID = 0;
		AZStd::string qString = "";
		std::shared_ptr<PLY::PLYResult> result;

		//Check query completed ok.
		PLY::PLYRequestBus::BroadcastResult(result, &PLY::PLYRequestBus::Events::GetResult, queryID);
		if (result == nullptr || result->errorType != PLY::PLYResult::NONE || result->errorMessage != "")
		{
			if (m_mode == SIMPLE)
			{
				PLYLOG(PLY::PLYLog::PLY_ERROR, "Benchmark failed. Couldn't create test data set. Stopping.");
			} 
			else if (m_mode == STARS)
			{
				PLYLOG(PLY::PLYLog::PLY_ERROR, "Benchmark failed. Couldn't check for star data set. Stopping.");
			}
			
			Stop();
			return;
		}

		if (m_mode == STARS)
		{
			if (result->resultSet.size() > 0)
			{
				pqxx::row row = result->resultSet.front();
				pqxx::field field = row.front();
				const char *f = field.c_str();

				//AZ_Printf("PLY", "gaia_main exists?: %s", f);

				if (strcmp(f, "t")!=0)
				{
					PLYLOG(PLY::PLYLog::PLY_ERROR, "Benchmark failed. Star data not present. Couldn't find table gaia_main. Stopping.");
					Stop();
					return;
				}
			}
			else
			{
				PLYLOG(PLY::PLYLog::PLY_ERROR, "Benchmark failed. Star data not present. Couldn't find table gaia_main. Stopping.");
		
				Stop();
				return;
			}
		}

		PLY::PLYRequestBus::Broadcast(&PLY::PLYRequestBus::Events::RemoveResult, queryID);

		if (m_mode == SIMPLE)
		{
			PLYLOG(PLY::PLYLog::PLY_INFO, "Benchmark running Analyze.");

			qString = "ANALYZE ply_test_data;";

			//Send query in NON TRANSACTION mode. VACUUM ANALYZE cannot be called inside a transaction block.
			PLY::PLYRequestBus::BroadcastResult(nextQueryID, &PLY::PLYRequestBus::Events::SendQueryNoTransaction, qString);
			if (nextQueryID == 0)
			{
				PLYLOG(PLY::PLYLog::PLY_ERROR, "Benchmark failed. Couldn't send create Analyze query. Stopping.");
				Stop();
				return;
			}

			m_vacuumAnalyzeQueryID = nextQueryID;
		}
		else if (m_mode == STARS)
		{
			//Run test queries on star data.

			PLYLOG(PLY::PLYLog::PLY_INFO, "Benchmark running test queries on star data.");

			//m_testDataset.clear();
			m_testDataRowCounts.clear();

			AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
			m_testStartTime = AZ::ScriptTimePoint(now);

			//Max stars to return PER QUERY.
			m_recordCount = 0; //Unused by this benchmark method.
			m_chunks = 100;

			for (int i = 0; i < m_chunks; ++i)
			{
				unsigned long long queryID = 0;

				//Select a random star, then use a bounding volume box around that star's position to search for more stars.
				//This keeps PostgreSQL from simply returning cached results for each query.
				//TABLESAMPLE SYSTEM_ROWS requires extension tsm_system_rows. Run SQL command "CREATE EXTENSION tsm_system_rows;" to add it.
				//tsm_system_rows provides the fastest method to obtain an arbitrary random row from a huge table.
				qString = "with rndstar as (select geom from gaia_main TABLESAMPLE SYSTEM_ROWS(1) limit 1) "
					"select ST_X(geom), ST_Y(geom), ST_Z(geom) from gaia_main where "
					"ST_3DMakeBox(ST_Translate((select geom from rndstar), -500, -500, -500), "
					"ST_Translate((select geom from rndstar), 500, 500, 500)) &&& geom limit 10000;";

				//AZ_Printf("PLY", "%s", qString.c_str());

				PLY::PLYRequestBus::BroadcastResult(queryID, &PLY::PLYRequestBus::Events::SendQueryWithOptions, qString, qs);

				if (queryID != 0) m_benchmarkQueryIDs.push_back(queryID);

				PLYLOG(PLY::PLYLog::PLY_DEBUG, "Sent query. Query ID returned was " + AZStd::string::format("%u", queryID));
			}
		}
	}

	//Is this the vacuum analyze query result?
	//Then run the benchmark query series.
	if (queryID == m_vacuumAnalyzeQueryID)
	{
		AZStd::string qString = "";
		std::shared_ptr<PLY::PLYResult> result;

		//Check query completed ok.
		PLY::PLYRequestBus::BroadcastResult(result, &PLY::PLYRequestBus::Events::GetResult, queryID);
		if (result == nullptr || result->errorType != PLY::PLYResult::NONE || result->errorMessage != "")
		{
			PLYLOG(PLY::PLYLog::PLY_ERROR, "Benchmark failed. Couldn't run Analyze. Stopping.");
			Stop();
			return;
		}
		else
		{
			PLY::PLYRequestBus::Broadcast(&PLY::PLYRequestBus::Events::RemoveResult, queryID);
		}

		PLYLOG(PLY::PLYLog::PLY_INFO, "Benchmark running test queries on sample data.");

		//m_testDataset.clear();
		m_testDataRowCounts.clear();

		AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
		m_testStartTime = AZ::ScriptTimePoint(now);

		//queryID will be set if the function was successful.
		m_chunks = 100;
		for (int i = 0; i < m_chunks; ++i)
		{
			unsigned long long queryID = 0;

			int rangeStart = i * (m_recordCount / m_chunks);
			int rangeEnd = rangeStart + (m_recordCount / m_chunks);

			qString = ("select rnd from ply_test_data where id > " + std::to_string(rangeStart)
				+ " and id < " + std::to_string(rangeEnd) + ";").c_str();

			PLY::PLYRequestBus::BroadcastResult(queryID, &PLY::PLYRequestBus::Events::SendQueryWithOptions, qString, qs);

			if (queryID != 0) m_benchmarkQueryIDs.push_back(queryID);

			PLYLOG(PLY::PLYLog::PLY_DEBUG, "Sent query. Query ID returned was " + AZStd::string::format("%u", queryID));
		}
	}

	//Is this queryID one of the benchmark test queries?
	if (std::find(m_benchmarkQueryIDs.begin(), m_benchmarkQueryIDs.end(), queryID) != m_benchmarkQueryIDs.end())
	{
		std::shared_ptr<PLY::PLYResult> result = nullptr;
		//Get a pointer to the result set.
		PLY::PLYRequestBus::BroadcastResult(result, &PLY::PLYRequestBus::Events::GetResult, queryID);
		if (result != nullptr)
		{

			PLYLOG(PLY::PLYLog::PLY_DEBUG, ("Benchmark received test query result ID " + AZStd::string::format("%u", queryID)).c_str());

			//Print benchmark result status to console.
			if (result->resultSet.size() > 0)
			{
				m_testDataRowCounts.push_back(static_cast<int>(result->resultSet.size()));
			}
			else if (result->errorType != PLY::PLYResult::NONE || result->errorMessage != "")
			{
				AZ_Printf("Script", "Query error: %s", result->errorMessage.c_str());
			}
			else if (m_mode == STARS)
			{
				//Empty dataset during stars benchmark. This is normal.
				m_testDataRowCounts.push_back(0);
			}
			else
			{
				AZ_Printf("Script", "%s", "Query error. Empty result set.");
			}

			//Tell PLY to delete the result object.
			PLY::PLYRequestBus::Broadcast(&PLY::PLYRequestBus::Events::RemoveResult, queryID);
			result = nullptr;
			
			if (m_testDataRowCounts.size() >= m_chunks)
			{
				AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
				m_testEndTime = AZ::ScriptTimePoint(now);

				AZ_Printf("Script", "%s", ("Test finished in (ms): "
					+ std::to_string(m_testEndTime.GetMilliseconds() - m_testStartTime.GetMilliseconds())
					+ " reaching " + std::to_string(STATS->GetMaxBusyThreadsOverall()) + " max busy worker threads.").c_str()
				);
				AZ_Printf("Script", "%s", (std::string("Benchmark returned ") + 
					std::to_string(std::accumulate(m_testDataRowCounts.begin(), m_testDataRowCounts.end(), 0.0f)) + " result rows.").c_str());
				
				//Append test data to benchmark results file.
				SaveFileData((std::string(m_filenamePrefix.c_str()) + "." + std::to_string(m_runID) + ".bch").c_str(), (std::to_string(m_testEndTime.GetMilliseconds() - m_testStartTime.GetMilliseconds()) + "\n").c_str(), true);
				
				if (m_curPass == m_passes - 1)
				{
					Stop();
					PLYLOG(PLY::PLYLog::PLY_INFO, "Benchmark finished.");
					m_done = true;
				}
				else
				{
					//Recursively run next benchmark test.
					Stop();
					m_curPass++;
					Run();
				}
			}

		}
	}

}

void PLY::Benchmark::SaveFileData(const AZStd::string fileName, const AZStd::string data, const bool append)
{
	using namespace AZ::IO;

	const void* dataBuffer = data.c_str();

	try
	{
		HandleType fileHandle = InvalidHandle;
		FileIOBase *f = FileIOBase::GetInstance();
		if (
			(!append && f->Open(("benchmark/" + fileName).c_str(), OpenMode::ModeWrite | OpenMode::ModeBinary, fileHandle))
			||
			(append && f->Open(("benchmark/" + fileName).c_str(), OpenMode::ModeAppend | OpenMode::ModeBinary, fileHandle))
		)
		{
			//Save data to the config file.
			f->Write(fileHandle, dataBuffer, data.size());

			f->Close(fileHandle);
		}
		else
		{
			PLYLOG(PLYLog::PLY_ERROR, "Couldn't save PLY benchmark results file. Unable to open file for saving.");
		}
	}
	catch (const std::exception &e)
	{
		PLYLOG(PLYLog::PLY_ERROR, "Couldn't save PLY benchmark results file. Error: " + AZStd::string(e.what()));
	}
}

void PLY::Benchmark::Startup()
{
	PLY::PLYResultBus::Handler::BusConnect();
}

void PLY::Benchmark::Shutdown()
{
	PLY::PLYResultBus::Handler::BusDisconnect();
}
