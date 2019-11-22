// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#define NOMINMAX
#define _AMD64_
#include <processthreadsapi.h>

#include "Worker.h"
#include <PLYSystemComponent.h>
#include <StatsCollector.h>
#include "PLYLog.h"

using namespace PLY;

PLY::Worker::Worker(PLY::PLYSystemComponent *psc, const unsigned long long &workerID, const PoolSettings::Priority &priority,
	const PoolSettings::WaitMode &waitMode, const int &reconnectWaitTime, const AZStd::string &connectionString)
	: m_workerID(workerID),
	m_psc(psc),
	m_c(nullptr),
	m_query(nullptr),
	m_busy(false),
	m_runQuery(false),
	m_shutdownThread(false),
	m_workerError(false),
	m_workerPriority(priority),
	m_waitMode(waitMode),
	m_reconnectWaitTime(reconnectWaitTime),
	m_connectionString(connectionString)
{
	m_workerThread = std::thread([this] { WorkerLoop(); });
}

PLY::Worker::~Worker()
{
	//Shut down thread.
	m_shutdownThread = true;
	if (m_workerThread.joinable())
	{
		PLYLOG(PLYLog::PLY_INFO, ("Thread cleaning itself up and waiting to join. Thread ID " + AZStd::string::format("%u", m_workerID)).c_str());
		m_workerThread.join();
		PLYLOG(PLYLog::PLY_INFO, ("Thread cleaning itself up JOINED. Thread ID " + AZStd::string::format("%u", m_workerID)).c_str());
	}
}

bool PLY::Worker::IsBusy() const
{
	return m_busy;
}

bool PLY::Worker::IsDead() const
{
	return m_workerError;
}

bool PLY::Worker::IsShutDown() const
{
	return m_shutdownThread;
}

void PLY::Worker::GiveQuery(std::shared_ptr<PLY::PLYQuery> query)
{
	m_busy = true;

	query->workerID = m_workerID;

	m_query = query;

	m_runQuery = true;

	STATS->AdjustBusyWorkersOverallStat(1);
}

std::shared_ptr<PLY::PLYQuery> PLY::Worker::GetQuery() const
{
	return m_query;
}

unsigned long long PLY::Worker::GetWorkerID() const
{
	return m_workerID;
}

void PLY::Worker::WorkerLoop()
{
	try
	{
		//Change priority of process. This must be set within the thread as it first starts.
		if (SetPriorityClass(GetCurrentProcess(), static_cast<DWORD>(m_workerPriority)))
		{
			PLYLOG(PLYLog::PLY_DEBUG, "Worker Thread process priority set to " + 
				AZStd::string::format("%u", static_cast<DWORD>(m_workerPriority)));
		}
		else
		{
			PLYLOG(PLYLog::PLY_ERROR,"Worker Thread Error - SetPriorityClass Error: " + AZStd::string::format("%u", GetLastError()));
			throw 1;
		}
		if (GetPriorityClass(GetCurrentProcess()) != static_cast<DWORD>(m_workerPriority))
		{
			PLYLOG(PLYLog::PLY_ERROR, "Worker Thread Error - Incorrect Priority");
			throw 1;
		}

		bool firstRun = true;
		while (!m_shutdownThread)
		{
			//Sleep or yield before looping. If we don't do this, the threads will happily eat the CPU for lunch.
			if (!firstRun)
			{
				if (m_waitMode == PoolSettings::SLEEP)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				else if (m_waitMode == PoolSettings::YIELD)
				{
					std::this_thread::yield();
				}
				else
				{
					PLYLOG(PLYLog::PLY_ERROR, "Worker Thread Error - Unknown wait mode");
					throw 1;
				}
			}

			firstRun = false;

			if (m_c == nullptr)
			{
				//Establish connection.
				try
				{

					//Establish connection.
					m_c = std::make_unique<pqxx::connection>(m_connectionString.c_str());
					PLYLOG(PLYLog::PLY_DEBUG, "DB connection established OK.");
				}
				catch (const pqxx::failure &e)
				{
					PLYLOG(PLYLog::PLY_ERROR, "Connection error: " + AZStd::string(e.what()));

					//Clean up connection object before trying again.
					m_c = nullptr;

					//Reset thread loop and try again.
					std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnectWaitTime));
					continue;
				}
			}

			if (m_runQuery && m_query != nullptr)
			{
				std::shared_ptr <PLY::PLYResult> result = nullptr;

				try
				{

					//Create empty result.
					result = std::make_shared<PLY::PLYResult>();

					//Copy queryID to the result.
					result->queryID = m_query->queryID;

					//Transfer settings from the query to the result.
					result->settings = m_query->settings;

					//Record query creation time.
					result->queryCreationTime = m_query->creationTime;

					//Record query start time.
					AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
					result->queryStartTime = AZ::ScriptTimePoint(now);

					//Run query and get results here.
					//AUTOMATIC TRANSACTIONS DISABLED WHILE A LIPQXX LIBRARY BUG IS BEING RESOLVED
					//See Github ticket #42 https://github.com/ash-j-f/PLY/issues/42
					/*if (m_query->settings.useTransaction)
					{
						pqxx::work w(*m_c);
						result->resultSet = w.exec(m_query->queryString.c_str());
						w.commit();
					}*/
					//else
					//{
						pqxx::nontransaction w(*m_c);
						result->resultSet = w.exec(m_query->queryString.c_str());
					//}
				}
				catch (const pqxx::broken_connection &e)
				{
					//Connection failure.
					
					//Abort and destroy the broken connection object.

					PLYLOG(PLYLog::PLY_ERROR, "SQL error: " + AZStd::string(e.what()));

					//Clean up connection object.
					m_c = nullptr;

					//Clean up result object.
					result = nullptr;

					//Reset thread loop and try again.
					std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnectWaitTime));
					continue;
				}
				catch (const pqxx::pqxx_exception &e)
				{
					//SQL failure.

					//Clean up result object.
					result = nullptr;

					//Place new empty result on queue with error message attached.
					result = std::make_shared<PLY::PLYResult>();

					//Copy queryID to the result.
					result->queryID = m_query->queryID;

					//Transfer settings from the query to the result.
					result->settings = m_query->settings;

					result->errorType = PLY::PLYResult::ResultErrorType::SQL_ERROR;

					result->errorMessage = e.base().what();
					
					PLYLOG(PLYLog::PLY_ERROR, "SQL error: " + AZStd::string(e.base().what()));
				}
				catch (const std::exception &e)
				{
					//Clean up result object.
					result = nullptr;
					
					PLYLOG(PLYLog::PLY_ERROR, "SQL error: " + AZStd::string(e.what()));

					//This is fatal. Re-throw.
					throw;
				}

				//Store generated result in results list.
				if (result != nullptr)
				{
					//Record query end time.
					AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
					result->queryEndTime = AZ::ScriptTimePoint(now);

					//Try to add result to the results queue.
					//If a result with the same query ID is already in the queue, we can just abandon the result object.
					if (m_psc->AddResult(result))
					{
						//Mark the query finished if it was added to the queue successfully.
						m_query->finished = true;
					}

					//Clear worker's pointer to the query object.
					m_query = nullptr;

					//Reset run flag.
					m_runQuery = false;

					//Reset busy flag. The worker is now ready for the next query.
					m_busy = false;

					STATS->AdjustBusyWorkersOverallStat(-1);
				}
			}
		}
	}
	catch (const std::exception &e)
	{
		PLYLOG(PLYLog::PLY_ERROR, ("Thread died. Error: " + AZStd::string(e.what()) + ". Thread ID " + AZStd::string::format("%u", m_workerID)).c_str());
		m_workerError = true;
	}
	catch (...)
	{
		PLYLOG(PLYLog::PLY_ERROR, ("Thread died. Unhandled exception. Thread ID " + AZStd::string::format("%u", m_workerID)).c_str());
		m_workerError = true;
	}
}
