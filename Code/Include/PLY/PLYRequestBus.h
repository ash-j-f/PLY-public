// PLY Gem request EBusTraits ebus. Used by projects to communicate with the PLY Gem's core functionality.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <PLY/PLYTypes.h>

#include <AzCore/EBus/EBus.h>

namespace PLY
{
	class PLYRequests
		: public AZ::EBusTraits
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// EBusTraits overrides
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
		static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
		//////////////////////////////////////////////////////////////////////////

		//Is the Libpq library built in thread safe mode?
		virtual bool GetLibpqThreadsafe() = 0;

		//Initialise the query worker pool.
		virtual void InitialisePool() = 0;

		//De-initialise the query worker pool.
		virtual void DeInitialisePool() = 0;

		//Add a query to the query queue, using default query options. If the query worker pool is initilised, it will be processed as soon as possible.
		//Will use automatic database transactions. Do NOT use BEGIN and COMMIT or other transaction keywords.
		//See "SendQueryNoTransaction" if you want to manage transactions yourself.
		//@param query The SQL string to use for the query.
		virtual unsigned long long SendQuery(const AZStd::string query) = 0;

		//Add a query to the query queue, without using automatic transactions. 
		//If the query worker pool is initilised, it will be processed as soon as possible.
		//@param query The SQL string to use for the query.
		virtual unsigned long long SendQueryNoTransaction(const AZStd::string query) = 0;

		//Add a query to the query queue, using custom query options. If the query worker pool is initilised, 
		//it will be processed as soon as possible.
		//Will use automatic database transactions. Do NOT use BEGIN and COMMIT or other transaction keywords.
		//See "SendQueryNoTransaction" if you want to manage transactions yourself.
		//@param query The SQL string to use for the query.
		//@param qs The query settings.
		virtual unsigned long long SendQueryWithOptions(const AZStd::string query, const PLY::QuerySettings qs) = 0;

		//Get a query result set from the results queue based on a query ID.
		//@param queryID The ID of the query used to create the results set.
		virtual std::shared_ptr<PLY::PLYResult> GetResult(const unsigned long long queryID) = 0;

		//Remove a result set from the results queue based on its query ID.
		//@param queryID The ID of the query used to create the results set.
		virtual void RemoveResult(const unsigned long long queryID) = 0;

		//Start a benchmark process using the "simple" test dataset.
		virtual void StartBenchmarkSimple() = 0;

		//Start a benchmark process using the GAIA STARS dataset.
		virtual void StartBenchmarkStars() = 0;

		//Star a benchmark sequence of multiple test runs using the GAIA STARS dataset.
		virtual void StartBenchmarkStarsSequence() = 0;

		//Stop the benchmark currently in progress.
		virtual void StopBenchmark() = 0;

		//Set the number of passes to use for the benchmark process.
		//@param passes The number of benchmark passes.
		virtual void SetBenchmarkPasses(int passes) = 0;
	};
	using PLYRequestBus = AZ::EBus<PLYRequests>;
} // namespace PLY
