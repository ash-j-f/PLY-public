// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include <regex>

#define NOMINMAX
#define _AMD64_
#include <processthreadsapi.h>

#include <PLY/PLYConfiguration.hpp>
#include <WorkManager.h>
#include <Worker.h>
#include <PLYSystemComponent.h>
#include "PLYLog.h"
#include <StatsCollector.h>

using namespace PLY;

PLY::WorkManager::WorkManager(PLY::PLYSystemComponent *psc)
	: m_psc(psc),
	m_shutdownThread(false),
	m_workManagerError(false)
{
	m_workManagerThread = std::thread([this] { WorkManagerLoop(); });
}

PLY::WorkManager::~WorkManager()
{
	//Shut down thread.
	m_shutdownThread = true;
	if (m_workManagerThread.joinable()) m_workManagerThread.join();
}

bool PLY::WorkManager::IsDead() const
{
	return m_workManagerError;
}

bool PLY::WorkManager::IsShutDown() const
{
	return m_shutdownThread;
}

void PLY::WorkManager::WorkManagerLoop()
{
	try
	{
		//Change priority of process. This must be set within the thread as it first starts.
		PoolSettings::Priority priority = PLYCONF->GetPoolSettings().managerPriority;
		if (SetPriorityClass(GetCurrentProcess(), static_cast<DWORD>(priority)))
		{
			PLYLOG(PLYLog::PLY_DEBUG, "WorkManager Thread process priority set to " + 
				AZStd::string::format("%u", static_cast<DWORD>(priority)));
		}
		else
		{
			PLYLOG(PLYLog::PLY_ERROR, "WorkManager Thread Error - SetPriorityClass Error: " + AZStd::string::format("%u", GetLastError()));
			throw 1;
		}
		if (GetPriorityClass(GetCurrentProcess()) != static_cast<DWORD>(priority))
		{
			PLYLOG(PLYLog::PLY_ERROR, "WorkManager Thread Error - Incorrect Priority");
			throw 1;
		}

		while (!m_shutdownThread)
		{

			//Find queries that have been on the queue too long and convert them to a result with a timeout error.
			std::unique_lock<std::mutex> lockQ1(m_psc->m_queryQueueMutex);
			AZStd::chrono::system_clock::time_point now1 = AZStd::chrono::system_clock::now();
			AZ::ScriptTimePoint currentTime1 = AZ::ScriptTimePoint(now1);
			for (std::list <std::shared_ptr<PLY::PLYQuery>>::iterator it = m_psc->m_queryQueue.begin(); it != m_psc->m_queryQueue.end();)
			{
				//A TTL of 0 means no TTL is enforced.
				if ((*it)->settings.queryTTL != 0 && 
					currentTime1.GetMilliseconds() - (*it)->creationTime.GetMilliseconds() > (*it)->settings.queryTTL)
				{
					AZ_Printf("WorkManager", "%s", ("Query " + AZStd::string::format("%u", (*it)->queryID) + " TTL expired ").c_str());
					
					PLYLOG(PLYLog::PLY_INFO, "Query " + AZStd::string::format("%u", (*it)->queryID) + " TTL expired");

					std::shared_ptr<PLY::PLYResult> result = std::make_shared<PLY::PLYResult>();

					//Copy queryID to the result.
					result->queryID = (*it)->queryID;

					//Transfer settings from the query to the result.
					result->settings = (*it)->settings;

					result->errorType = PLY::PLYResult::ResultErrorType::TTL_EXPIRED;

					result->errorMessage = "Result TTL expired.";

					//Try to add result to the results queue.
					if (!m_psc->AddResult(result))
					{
						//Discard the results object if there is already one in the results queue with this queryID.
						result = nullptr;
					}
					else
					{
						//Mark the query finished.
						(*it)->finished = true;
					}

					it = m_psc->m_queryQueue.erase(it);
				}	
				else
				{
					++it;
				}
			}
			lockQ1.unlock();

			//Find results that have been on the queue too long and remove them.
			std::unique_lock<std::mutex> lockR1(m_psc->m_resultsQueueMutex);
			AZStd::chrono::system_clock::time_point now2 = AZStd::chrono::system_clock::now();
			AZ::ScriptTimePoint currentTime2 = AZ::ScriptTimePoint(now2);
			for (std::map<unsigned long long, std::shared_ptr<PLY::PLYResult>>::iterator it = m_psc->m_resultsQueue.begin(); 
				it != m_psc->m_resultsQueue.end();)
			{

				//A TTL of 0 means no TTL is enforced.
				if ((*it).second->settings.resultTTL != 0 &&
					currentTime2.GetMilliseconds() - (*it).second->resultCreationTime.GetMilliseconds() > (*it).second->settings.resultTTL)
				{			
					PLYLOG(PLYLog::PLY_INFO, "Result " + AZStd::string::format("%u", (*it).second->queryID) + " TTL expired");

					it = m_psc->m_resultsQueue.erase(it);
				}
				else
				{
					++it;
				}
			}
			lockR1.unlock();

			//Look for dead workers, kill their thread and allow the query to be sent to a new thread.
			std::unique_lock<std::mutex> lockW2(m_psc->m_workersMutex);
			for (std::vector <std::shared_ptr<PLY::Worker>>::iterator it = m_psc->m_workers.begin(); it != m_psc->m_workers.end();)
			{
				if ((*it)->IsDead() || (*it)->IsShutDown())
				{	
					PLYLOG(PLYLog::PLY_WARNING, "Dead or shut down worker found.");

					std::shared_ptr<PLY::PLYQuery> pq = (*it)->GetQuery();

					//Was this worker running a query?
					if (pq != nullptr)
					{
						//Free up the query to be assigned to a new worker.
						std::unique_lock<std::mutex> lockQ(m_psc->m_queryQueueMutex);
						pq->workerID = 0;
						lockQ.unlock();
						STATS->AdjustBusyWorkersOverallStat(-1);
					}

					PLYLOG(PLYLog::PLY_DEBUG, "Worker queue size before removal " + AZStd::string::format("%u", m_psc->m_workers.size()));

					//Remove worker from workers list.
					it = m_psc->m_workers.erase(it);

					PLYLOG(PLYLog::PLY_DEBUG, "Worker queue size after removal " + AZStd::string::format("%u", m_psc->m_workers.size()));
				}
				else
				{
					++it;
				}
			}
			lockW2.unlock();

			//Find any queries that need workers, and assign them to workers.
			//Check for new queries, and give them to connections in the pool.
			//Establish lock on queue.
			std::unique_lock<std::mutex> lockQ2(m_psc->m_queryQueueMutex);
			for (auto &q : m_psc->m_queryQueue)
			{

				//Skip queries already assigned to workers.
				if (q->workerID != 0) continue;

				//Skip queries already finished.
				if (q->finished) continue;

				bool gaveQuery = false;

				//Find a worker that's not busy and assign the query to it, if possible.
				std::unique_lock<std::mutex> lockW1(m_psc->m_workersMutex);
				for (auto &w : m_psc->m_workers)
				{
					if (!w->IsBusy() && !w->IsDead())
					{
						w->GiveQuery(q);
						gaveQuery = true;
						break;
					}
				}
				lockW1.unlock();

				if (!gaveQuery)
				{
					//No workers were available, so start a new one and assign the query to it, if possible.
					std::unique_lock<std::mutex> lockW2(m_psc->m_workersMutex);

					PoolSettings p = PLYCONF->GetPoolSettings();

					if (m_psc->m_workers.size() < p.maxPoolSize)
					{
						std::shared_ptr<PLY::Worker> w = std::make_shared<PLY::Worker>(m_psc, m_psc->GetNextWorkerID(), 
							p.workerPriority, p.waitMode, PLYCONF->GetDatabaseConnectionDetails().reconnectWaitTime,
							PLYCONF->GetConnectionString());
						m_psc->m_workers.push_back(w);
						w->GiveQuery(q);
						gaveQuery = true;
					}
					lockW2.unlock();
				}

				if (gaveQuery)
				{
					PLYLOG(PLYLog::PLY_DEBUG, "Gave query to thread");
				}
				else
				{
					//No workers were available, and we couldn't create new workers, so abandon trying to assign queries to workers for now.
					break;
				}
			}

			//Delete queries already finished.
			for (std::list <std::shared_ptr<PLY::PLYQuery>>::iterator it = m_psc->m_queryQueue.begin(); it != m_psc->m_queryQueue.end();)
			{
				if ((*it)->finished)
				{
					PLYLOG(PLYLog::PLY_DEBUG, "Query removing ID " + AZStd::string::format("%u", (*it)->queryID));
					it = m_psc->m_queryQueue.erase(it);
					PLYLOG(PLYLog::PLY_DEBUG, "Query queue size " + AZStd::string::format("%u", m_psc->m_queryQueue.size()));
				}
				else
				{
					++it;
				}
			}

			//Unlock query queue ASAP so we don't block other threads.
			lockQ2.unlock();

			//Sleep before looping again. If we don't do this, the threads will happily eat the CPU for lunch.
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	catch (const std::exception &e)
	{
		PLYLOG(PLYLog::PLY_ERROR, "WorkManager thread died. Error: " + AZStd::string(e.what()));
		m_workManagerError = true;
	}
	catch (...)
	{
		PLYLOG(PLYLog::PLY_ERROR, "WorkManager thread died. Unhandled exception.");
		m_workManagerError = true;
	}
}
