// System component for the PLY Gem. This component must be added to a project before it can access the PLY Gem functionality.
// This component provides the core PLY Gem functionality.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <PLY/PLYTools.h>
#include <PLY/PLYTypes.h>
#include <PLY/PLYRequestBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

//Forward declarations to allow test system to access this class.
class PLYTest;
class PLYTest_LibpqThreadSafe_Test;

namespace PLY
{
	//Forward declarations.
	class Worker;
	class WorkManager;
	class Benchmark;
	class Console;

    class PLYSystemComponent
        : public AZ::Component,
        protected PLYRequestBus::Handler,
		public AZ::TickBus::Handler
    {

	friend PLYTest;
	friend PLYTest_LibpqThreadSafe_Test;
	friend Worker;
	friend WorkManager;
	friend Benchmark;

    public:
		
		//Default constructor.
		PLYSystemComponent();

		//Default destructor.
		~PLYSystemComponent();

        AZ_COMPONENT(PLYSystemComponent, "{9D86CFD9-9679-42CB-9CB8-D521A45C565E}");
		
		//Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

		// Optional functions for defining provided and dependent services.
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
		static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:

		////////////////////////////////////////////////////////////////////////
		// PLY interfaces
		////////////////////////////////////////////////////////////////////////

		//Is the Libpq library built in thread safe mode?
		bool GetLibpqThreadsafe() override;
		
		//Initialise the query worker pool.
		void InitialisePool() override;

		//De-initialise the query worker pool.
		void DeInitialisePool() override;

		//Add a query to the query queue, using default query options. If the query worker pool is initilised, it will be processed as soon as possible.
		//Will use automatic database transactions. Do NOT use BEGIN and COMMIT or other transaction keywords.
		//See "SendQueryNoTransaction" if you want to manage transactions yourself.
		//@param query The SQL string to use for the query.
		unsigned long long SendQuery(const AZStd::string query) override;

		//Add a query to the query queue, without using automatic transactions. 
		//If the query worker pool is initilised, it will be processed as soon as possible.
		//@param query The SQL string to use for the query.
		unsigned long long SendQueryNoTransaction(const AZStd::string query) override;

		//Add a query to the query queue, using custom query options. If the query worker pool is initilised, 
		//it will be processed as soon as possible.
		//Will use automatic database transactions. Do NOT use BEGIN and COMMIT or other transaction keywords.
		//See "SendQueryNoTransaction" if you want to manage transactions yourself.
		//@param query The SQL string to use for the query.
		//@param qs The query settings.
		unsigned long long SendQueryWithOptions(const AZStd::string query, const PLY::QuerySettings qs) override;

		//Get a query result set from the results queue based on a query ID.
		//@param queryID The ID of the query used to create the results set.
		std::shared_ptr<PLY::PLYResult> GetResult(const unsigned long long queryID) override;
		
		//Remove a result set from the results queue based on its query ID.
		//@param queryID The ID of the query used to create the results set.
		void RemoveResult(const unsigned long long queryID) override;

		//Start a benchmark process using the "simple" test dataset.
		void StartBenchmarkSimple() override;

		//Start a benchmark process using the GAIA STARS dataset.
		void StartBenchmarkStars() override;

		//Star a benchmark sequence of multiple test runs using the GAIA STARS dataset.
		void StartBenchmarkStarsSequence() override;

		//Stop the benchmark currently in progress.
		void StopBenchmark() override;

		//Set the number of passes to use for the benchmark process.
		//@param passes The number of benchmark passes.
		void SetBenchmarkPasses(int passes) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

	private:

		//Mutex to lock list of query worker threads while it is modified.
		std::mutex m_workersMutex;
		//Query worker threads.
		std::vector<std::shared_ptr<PLY::Worker>> m_workers;

		//Next unqiue connection worker ID.
		unsigned long long m_nextWorkerID;

		//Mutex to lock query queue while it is modified.
		std::mutex m_queryQueueMutex;
		//Query queue.
		std::list <std::shared_ptr<PLY::PLYQuery>> m_queryQueue;

		//Mutex to lock query queue while it is modified.
		std::mutex m_resultsQueueMutex;
		//Results queue.
		std::map<unsigned long long, std::shared_ptr<PLY::PLYResult>> m_resultsQueue;

		//Unqiue query IDs.
		unsigned long long m_nextQueryID;

		//Has the query worker pool been initialised?
		bool m_poolInitialised;

		//Work manager.
		std::unique_ptr<WorkManager> m_workManager;

		//Benchmark object.
		std::unique_ptr<Benchmark> m_benchmark;

		//Number of benchmark passes to run.
		int m_benchmarkPasses;

		//Is the benchmark sequence running?
		bool m_benchmarkSequenceRunning;

		//Current benchmark sequence step.
		int m_benchmarkSequenceStep;

		//Original query worker pool settings before benchmark was run.
		//Used to restore settings after benchmark run (which may modify settings as part of a benchmark sequence).
		PLY::PoolSettings m_poolSettingsOriginal;

		//Original database connection settings before benchmark was run.
		//Used to restore settings after benchmark run (which may modify settings as part of a benchmark sequence).
		PLY::DatabaseConnectionDetails m_databaseConnectionDetailsOriginal;

		//Have console commands been registered?
		bool m_registeredConsoleCommands;

		//Console command manager.
		std::unique_ptr<Console> m_consoleCommandManager;

		//Get the next query worker thread ID.
		unsigned long long GetNextWorkerID();

		//Tick order definition. This value sets where in global tick order this component is called.
		//TICK_PLACEMENT is fairly early in the tick order.
		//TICK_DEFAULT is the default position for components.
		//TICK_GAME is recommended for "game related components".
		//Consider that when PLY advertises query results, it is activating event responses in game 
		//components, so TICK_GAME is probably the best choice here.
		inline int GetTickOrder() override { return AZ::ComponentTickBus::TICK_GAME; };

		//Tick handler.
		void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

		//Run benchmark sequence update actions.
		void BenchmarkSequenceUpdate();

		//Clean up all threads and pools.
		void Cleanup();

		//Add a result to the results queue.
		//@param result The result to add to the queue.
		bool AddResult(std::shared_ptr <PLY::PLYResult> result);

    };
}
