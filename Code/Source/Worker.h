// Query worker thread. Workers are given queries from thr query queue. They then attempt to connect to the database
// (if no connection already exists), and process the query. Query results are placed on the results queue.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <PLY/PLYTools.h>
#include <PLY/PLYTypes.h>

namespace PLY
{
	//Forward declarations.
	class PLYSystemComponent;

	class Worker
	{
	public:
		Worker(PLY::PLYSystemComponent *psc, const unsigned long long &workerID, const PoolSettings::Priority &priority,
			const PoolSettings::WaitMode &waitMode, const int &reconnectWaitTime, const AZStd::string &connectionString);
		~Worker();

		//Is this worker busy?
		bool IsBusy() const;

		//Has this worker died?
		bool IsDead() const;

		//Has this worker been shut down?
		bool IsShutDown() const;

		//Give query to this worker from the query queue.
		void GiveQuery(std::shared_ptr<PLY::PLYQuery> query);

		//Get the query this worker is currently working on.
		std::shared_ptr<PLY::PLYQuery> GetQuery() const;

		//Get the worker's unique ID.
		unsigned long long GetWorkerID() const;

	private:

		//The worker ID.
		unsigned long long m_workerID;

		//Is this connection busy?
		std::atomic<bool> m_busy;

		//Should the thread run the given query?
		std::atomic<bool> m_runQuery;

		//Command the thread to shut down.
		std::atomic<bool> m_shutdownThread;

		//Was there an unrecoverable error with the thread?
		std::atomic<bool> m_workerError;

		//Query.
		std::shared_ptr<PLY::PLYQuery> m_query;
		
		//Database connection.
		std::unique_ptr<pqxx::connection> m_c;

		//Pointer to PLYSystemComponent that owns the connections and queues.
		PLY::PLYSystemComponent *m_psc;

		//Thread for the query worker.
		std::thread m_workerThread;

		//Worker priority setting.
		PoolSettings::Priority m_workerPriority;

		//Worker wait mode stting.
		PoolSettings::WaitMode m_waitMode;

		//Worker reconnect wait time setting.
		int m_reconnectWaitTime;

		//Worker database connection string.
		AZStd::string m_connectionString;

		//Main thread work function.
		void WorkerLoop();
	};
}