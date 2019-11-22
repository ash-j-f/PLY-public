// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include <functional>

#include <PLYSystemComponent.h>
#include <Worker.h>
#include <WorkManager.h>
#include <Benchmark.h>
#include <PLY/PLYConfiguration.hpp>
#include <PLY/PLYResultBus.h>
#include <StatsCollector.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include "Console.h"

namespace PLY
{
	PLYSystemComponent::PLYSystemComponent()
		: m_nextQueryID(1),
		m_nextWorkerID(1),
		m_poolInitialised(false),
		m_registeredConsoleCommands(false),
		m_benchmarkPasses(1),
		m_benchmarkSequenceRunning(false),
		m_benchmarkSequenceStep(0)
	{
	}

	PLYSystemComponent::~PLYSystemComponent()
	{
		Cleanup();
	}

	void PLYSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PLYSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
				
				ec->Class<PLYSystemComponent>("PLY", "PostgreSQL Database Connector")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
					->Attribute(AZ::Edit::Attributes::AutoExpand, true)
					;
				
            }
        }
    }

    void PLYSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PLYService"));
    }

    void PLYSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PLYService"));
    }

    void PLYSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void PLYSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

	bool PLYSystemComponent::AddResult(std::shared_ptr <PLY::PLYResult> result)
	{
		//Establish lock on queue. Lock is released as it goes out of scope.
		std::unique_lock<std::mutex> lock(m_resultsQueueMutex);

		//Only record this result if a result for this queryID doesn't already exist.
		if (m_resultsQueue.find(result->queryID) == m_resultsQueue.end())
		{
			m_resultsQueue[result->queryID] = result;

			STATS->CountResult();

			return true;
		}

		return false;
	}

	unsigned long long PLYSystemComponent::SendQuery(const AZStd::string query)
	{
		//No query settings passed, so use configured defaults.
		return SendQueryWithOptions(query, QuerySettings());
	}

	unsigned long long PLYSystemComponent::SendQueryNoTransaction(const AZStd::string query)
	{
		//Turn off automatic transaction for this query.
		QuerySettings qs;
		qs.useTransaction = false;

		//Use other configured query setting defaults.
		return SendQueryWithOptions(query, qs);
	}

	unsigned long long PLYSystemComponent::SendQueryWithOptions(const AZStd::string query, const PLY::QuerySettings qs)
	{
		//Get next query ID.
		long long queryID = m_nextQueryID;

		//Increment query ID for next request.
		m_nextQueryID++;

		//Create query object.
		std::shared_ptr<PLY::PLYQuery> pq = std::make_shared<PLY::PLYQuery>();

		pq->queryID = queryID;

		pq->queryString = query;

		//Override default query settings with chosen values.
		pq->settings = qs;

		//Establish lock on queue. Lock is released as it goes out of scope.
		std::unique_lock<std::mutex> lock(m_queryQueueMutex);

		//Set the query creation time to now, so it accurately represents the time it was added to the queue.
		AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
		AZ::ScriptTimePoint currentTime = AZ::ScriptTimePoint(now);

		pq->creationTime = currentTime;

		//Add query to queue.
		m_queryQueue.push_back(pq);

		STATS->CountQuery();

		return queryID;
	}

	std::shared_ptr<PLY::PLYResult> PLYSystemComponent::GetResult(const unsigned long long queryID)
	{
		//Establish lock on queue. Lock is released as it goes out of scope.
		std::unique_lock<std::mutex> lock(m_resultsQueueMutex);
		
		if (m_resultsQueue.count(queryID) != 0) return m_resultsQueue[queryID];

		return nullptr;
	}

	/**
	* Delete the result object and remove it from results list.
	*/
	void PLYSystemComponent::RemoveResult(const unsigned long long queryID)
	{
		//Establish lock on queue. Lock is released as it goes out of scope.
		std::unique_lock<std::mutex> lock(m_resultsQueueMutex);

		std::map<unsigned long long, std::shared_ptr<PLY::PLYResult>>::iterator it = m_resultsQueue.find(queryID);

		if (it != m_resultsQueue.end())
		{
			PLYLOG(PLYLog::PLY_DEBUG, ("Result removed ID " + AZStd::string::format("%u", queryID)).c_str());
			PLYLOG(PLYLog::PLY_DEBUG, ("Result queue size " + AZStd::string::format("%u", m_resultsQueue.size())).c_str());

			m_resultsQueue.erase(it);
		}
	}

	void PLYSystemComponent::StartBenchmarkSimple()
	{
		m_benchmark = std::make_unique<Benchmark>(this, Benchmark::SIMPLE, m_benchmarkPasses, "benchmark");
		m_benchmark->Run();
	}

	void PLYSystemComponent::StartBenchmarkStars()
	{
		m_benchmark = std::make_unique<Benchmark>(this, Benchmark::STARS, m_benchmarkPasses, "benchmark");
		m_benchmark->Run();
	}

	void PLYSystemComponent::StartBenchmarkStarsSequence()
	{
		if (m_benchmarkSequenceRunning)
		{
			PLYLOG(PLYLog::PLY_ERROR, "Benchmark sequence already running.");
			return;
		}

		m_benchmark = nullptr;
		m_benchmarkSequenceRunning = true;
		m_benchmarkSequenceStep = 0;

	}

	void PLYSystemComponent::StopBenchmark()
	{
		if (m_benchmark != nullptr)
		{
			m_benchmark->Stop();
		}
		else
		{
			AZ_Printf("PLY", "%s", "Benchmark object not present. Benchmark has not been started.");
		}
		m_benchmark = nullptr;
		m_benchmarkSequenceRunning = false;
		m_benchmarkSequenceStep = 0;
	}

	void PLYSystemComponent::SetBenchmarkPasses(int passes)
	{
		AZ_Error("PLY", passes > 0, "Passes must be more than zero");
		m_benchmarkPasses = passes;
	}

	void PLYSystemComponent::Init()
    {
		
    }

    void PLYSystemComponent::Activate()
    {
        PLYRequestBus::Handler::BusConnect();
		AZ::TickBus::Handler::BusConnect();
    }

    void PLYSystemComponent::Deactivate()
    {
        PLYRequestBus::Handler::BusDisconnect();
		AZ::TickBus::Handler::BusDisconnect();
    }

	unsigned long long PLYSystemComponent::GetNextWorkerID()
	{
		//Get next worker ID.
		unsigned long long workerID = m_nextWorkerID;
		
		//Increment worker ID.
		m_nextWorkerID++;
		
		return workerID;
	}

	void PLYSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
	{

		//Execute stats tick.
		STATS->OnTick(deltaTime, time);

		//Register console commands here. For some reason this doesn't work if placed in PLYSystemComponent::Activate()
		if (!m_registeredConsoleCommands)
		{
			m_registeredConsoleCommands = true;

			//Register commands.
			m_consoleCommandManager = std::make_unique<Console>();
		}

		//Run query results advertising if pool is initialised.
		//This is done in OnTick as it has to be performed by the main thread.
		if (m_poolInitialised)
		{

			std::vector<std::shared_ptr<PLY::PLYResult>> advertise;

			//Establish lock on queue. Lock is released as it goes out of scope.
			std::unique_lock<std::mutex> lock(m_resultsQueueMutex);
			for (auto &r : m_resultsQueue)
			{
				//Find results that need advertising, and haven't yet been advertised.
				if (r.second->settings.advertiseResult && !r.second->hasBeenAdvertised)
				{
					//Collect a list of results that need to be advertised.
					advertise.push_back(r.second);
				}
			}
			//Unlock ASAP.
			lock.unlock();

			//Run advertising of results outside the locked block above, as processes may take 
			//a long time to do what they need with the advertised result.
			for (auto &r : advertise)
			{
				PLYLOG(PLYLog::PLY_DEBUG, "Advertising result");

				r->hasBeenAdvertised = true;
				PLY::PLYResultBus::Broadcast(&PLY::PLYResultBus::Events::ResultReady, r->queryID);
			}
		}

		//Check if work manager thread died, and restart it if required.
		if (m_poolInitialised && m_workManager != nullptr && (m_workManager->IsDead() || m_workManager->IsShutDown()))
		{
			PLYLOG(PLYLog::PLY_WARNING, "Detected that WorkManager is not running. Restarting WorkManager.");

			//Destroy the old work manager.
			m_workManager = nullptr;

			//Start up a new work manager.
			m_workManager = std::make_unique<WorkManager>(this);
		}

		BenchmarkSequenceUpdate();

	}

	void PLYSystemComponent::BenchmarkSequenceUpdate()
	{
		if (m_benchmarkSequenceRunning)
		{
			//Run Stars benchmark with selected options.
			auto f =
				[this](std::string name = "benchmark",
					int threads = 8,
					DatabaseConnectionDetails::SSLMode ssl = DatabaseConnectionDetails::SSLMode::PREFER,
					PoolSettings::WaitMode wait = PoolSettings::WaitMode::SLEEP,
					PoolSettings::Priority managerPriority = PoolSettings::Priority::NORMAL,
					PoolSettings::Priority workerPriority = PoolSettings::Priority::NORMAL)
			{
				DeInitialisePool();

				PLY::PoolSettings p = PLYCONF->GetPoolSettings();
				p.minPoolSize = threads;
				p.maxPoolSize = threads;
				p.waitMode = wait;
				p.managerPriority = managerPriority;
				p.workerPriority = workerPriority;
				PLYCONF->SetPoolSettings(p);

				PLY::DatabaseConnectionDetails d = PLYCONF->GetDatabaseConnectionDetails();
				d.sslMode = ssl;
				PLYCONF->SetDatabaseConnectionDetails(d);

				InitialisePool();

				m_benchmark = std::make_unique<Benchmark>(this, Benchmark::STARS, m_benchmarkPasses, name.c_str());
				m_benchmark->Run();
			};

			if (m_benchmarkSequenceStep == 0)
			{
				AZ_Printf("PLY", "%s", "Starting PLY benchmark sequence.");

				m_poolSettingsOriginal = PLYCONF->GetPoolSettings();
				m_databaseConnectionDetailsOriginal = PLYCONF->GetDatabaseConnectionDetails();

				//Threads [1 .. 9]...

				m_benchmarkSequenceStep = 1;

				f("benchmark.threads1", 1);
			}
			else if (m_benchmarkSequenceStep == 1 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 2;

				f("benchmark.threads2", 2);
			}
			else if (m_benchmarkSequenceStep == 2 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 3;

				f("benchmark.threads3", 3);
			}
			else if (m_benchmarkSequenceStep == 3 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 4;

				f("benchmark.threads4", 4);
			}
			else if (m_benchmarkSequenceStep == 4 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 5;

				f("benchmark.threads5", 5);
			}
			else if (m_benchmarkSequenceStep == 5 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 6;

				f("benchmark.threads6", 6);
			}
			else if (m_benchmarkSequenceStep == 6 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 7;

				f("benchmark.threads7", 7);
			}
			else if (m_benchmarkSequenceStep == 7 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 8;

				f("benchmark.threads8", 8);
			}
			else if (m_benchmarkSequenceStep == 8 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 9;

				f("benchmark.threads9", 9);
			}
			else if (m_benchmarkSequenceStep == 9 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 10;

				//SSL OFF

				//Revert settings to original values.
				PLYCONF->SetPoolSettings(m_poolSettingsOriginal);
				PLYCONF->SetDatabaseConnectionDetails(m_databaseConnectionDetailsOriginal);

				f("benchmark.sslOFF", 8, DatabaseConnectionDetails::DISABLE);
			}
			else if (m_benchmarkSequenceStep == 10 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 11;

				//Loop mode YIELD

				//Revert settings to original values.
				PLYCONF->SetPoolSettings(m_poolSettingsOriginal);
				PLYCONF->SetDatabaseConnectionDetails(m_databaseConnectionDetailsOriginal);

				f("benchmark.modeYIELD", 8, DatabaseConnectionDetails::PREFER, PoolSettings::WaitMode::YIELD);
			}

			else if (m_benchmarkSequenceStep == 11 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 12;

				//Work manager priority below normal.

				//Revert settings to original values.
				PLYCONF->SetPoolSettings(m_poolSettingsOriginal);
				PLYCONF->SetDatabaseConnectionDetails(m_databaseConnectionDetailsOriginal);

				f("benchmark.managerBELOWNORMAL", 8, DatabaseConnectionDetails::PREFER, PoolSettings::SLEEP,
					PoolSettings::Priority::BELOW_NORMAL,
					PoolSettings::Priority::NORMAL);
			}
			else if (m_benchmarkSequenceStep == 12 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 13;

				//Work manager priority idle.

				//Revert settings to original values.
				PLYCONF->SetPoolSettings(m_poolSettingsOriginal);
				PLYCONF->SetDatabaseConnectionDetails(m_databaseConnectionDetailsOriginal);

				f("benchmark.managerIDLE", 8, DatabaseConnectionDetails::PREFER, PoolSettings::SLEEP,
					PoolSettings::Priority::IDLE,
					PoolSettings::Priority::NORMAL);

			}
			else if (m_benchmarkSequenceStep == 13 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 14;

				//Worker priority below normal.

				//Revert settings to original values.
				PLYCONF->SetPoolSettings(m_poolSettingsOriginal);
				PLYCONF->SetDatabaseConnectionDetails(m_databaseConnectionDetailsOriginal);

				f("benchmark.workerBELOWNORMAL", 8, DatabaseConnectionDetails::PREFER, PoolSettings::SLEEP,
					PoolSettings::Priority::NORMAL,
					PoolSettings::Priority::BELOW_NORMAL);
			}
			else if (m_benchmarkSequenceStep == 14 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				m_benchmarkSequenceStep = 15;

				//Worker priority below idle.

				//Revert settings to original values.
				PLYCONF->SetPoolSettings(m_poolSettingsOriginal);
				PLYCONF->SetDatabaseConnectionDetails(m_databaseConnectionDetailsOriginal);

				f("benchmark.workerIDLE", 8, DatabaseConnectionDetails::PREFER, PoolSettings::SLEEP,
					PoolSettings::Priority::NORMAL,
					PoolSettings::Priority::IDLE);
			}

			else if (m_benchmarkSequenceStep == 15 && (m_benchmark == nullptr || m_benchmark->IsFinished()))
			{
				//FINISH

				//Revert settings to original values.
				PLYCONF->SetPoolSettings(m_poolSettingsOriginal);
				PLYCONF->SetDatabaseConnectionDetails(m_databaseConnectionDetailsOriginal);

				m_benchmarkSequenceRunning = false;
				m_benchmarkSequenceStep = 0;
				m_benchmark = nullptr;

				AZ_Printf("PLY", "%s", "PLY benchmark sequence finished.");
			}
		}
	}

	void PLYSystemComponent::Cleanup()
	{

		//Clean up work manager.
		m_workManager = nullptr;

		//Clean up connections.
		std::unique_lock<std::mutex> lockC(m_workersMutex);
		m_workers.clear();
		lockC.unlock();

		//Clean up query queue.
		std::unique_lock<std::mutex> lockQ(m_queryQueueMutex);
		m_queryQueue.clear();
		lockQ.unlock();

		//Clean up results queue.
		std::unique_lock<std::mutex> lockR(m_resultsQueueMutex);
		m_resultsQueue.clear();
		lockR.unlock();

		//Reset next available query ID.
		m_nextQueryID = 1;

		//Reset next available worker ID.
		m_nextWorkerID = 1;
	}

	bool PLYSystemComponent::GetLibpqThreadsafe()
	{
		pqxx::thread_safety_model tsm = pqxx::describe_thread_safety();
		return tsm.safe_libpq;
	}

	void PLYSystemComponent::InitialisePool()
	{
		//Only allow initialisation once.
		if (m_poolInitialised) return;

		//Create worker threads, up to the established minimum number.
		//Establish lock on queue.
		std::unique_lock<std::mutex> lock(m_workersMutex);

		for (int i = 0; i < PLYCONF->GetPoolSettings().minPoolSize; i++)
		{
			m_workers.push_back(std::make_shared<PLY::Worker>(this, GetNextWorkerID(), PLYCONF->GetPoolSettings().workerPriority,
				PLYCONF->GetPoolSettings().waitMode, PLYCONF->GetDatabaseConnectionDetails().reconnectWaitTime, PLYCONF->GetConnectionString()));
		}
		//Unlock ASAP.
		lock.unlock();

		//Create work manager thread.
		m_workManager = std::make_unique<WorkManager>(this);

		m_poolInitialised = true;

		PLYLOG(PLYLog::PLY_INFO, "PLY system Pool Initialised");
	}

	void PLYSystemComponent::DeInitialisePool()
	{
		//Abort if pool not initialised.
		if (!m_poolInitialised) return;

		Cleanup();

		m_poolInitialised = false;

		PLYLOG(PLYLog::PLY_INFO, "PLY system Pool De-initialised");
	}

}
