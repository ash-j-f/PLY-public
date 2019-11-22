// Work manager thread. The work manager performs the following tasks: 
// - monitor the query queue and hand new queries to query worker threads
// - remove queries from the query queue that have been successfully processed by a worker
// - monitor the query and results queues and remove queries and results that have exceeded their TTL (Time To Live)
// - detect dead worker threads and clean them up
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <PLY/PLYTools.h>
#include <PLY/PLYTypes.h>

namespace PLY
{
	//Forward declarations.
	class PLYSystemComponent;

	class WorkManager
	{

	public:
		WorkManager(PLY::PLYSystemComponent *psc);
		~WorkManager();

		bool IsDead() const;
		bool IsShutDown() const;

	private:

		//Command the thread to shut down.
		std::atomic<bool> m_shutdownThread;

		//Was there an unrecoverable error with the thread?
		std::atomic<bool> m_workManagerError;

		//Pointer to PLYSystemComponent that owns the connections and queues.
		PLY::PLYSystemComponent *m_psc;

		//Main thread.
		std::thread m_workManagerThread;

		//Main thread function.
		void WorkManagerLoop();
	};
}