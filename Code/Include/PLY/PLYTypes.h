// Common structs used by many PLY Gem classes.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#ifndef PQXX_H_
#define PQXX_H_
#include <pqxx/pqxx>
#endif

#include <AzCore/std/string/string.h>

#include <AzCore/Script/ScriptTimePoint.h>

namespace PLY
{

	//Database connection details.
	struct DatabaseConnectionDetails
	{
	public:

		enum SSLMode { DISABLE=0, ALLOW=1, PREFER=2, REQUIRE=3, VERIFY_CA=4, VERIFY_FULL=5 };

		DatabaseConnectionDetails()
			: port(5432),
			host("localhost"),
			database(""),
			username(""),
			password(""),
			reconnectWaitTime(200), //Milliseconds
			connectTimeout(0), //Seconds. 0 = use Libpq default.
			sslMode(SSLMode::PREFER) //Prefer is the Libpq default.
		{};
		~DatabaseConnectionDetails() {};

		int port;
		AZStd::string host;
		AZStd::string database;
		AZStd::string username;
		AZStd::string password;
		int reconnectWaitTime; //Milliseconds
		int connectTimeout; //Seconds. 0 = use Libpq default.
		SSLMode sslMode;
	};

	//Query settings.
	struct QuerySettings
	{
	public:

		QuerySettings() :
			advertiseResult(true),
			queryTTL(0), //Milliseconds. 0 means no TTL is enforced.
			resultTTL(0), //Milliseconds. 0 means no TTL is enforced.
			useTransaction(true)
		{};
		~QuerySettings() {};
		
		//Should the PLY module advertise query results via the query results bus?
		bool advertiseResult;
		//Time (milliseconds) a query can remain in the query queue before being deleted automatically. 0 means no TTL is enforced.
		int queryTTL;
		//Time (milliseconds) a query can remain in the results queue before being deleted automatically. 0 means no TTL is enforced.
		int resultTTL;
		//Use automatic transaction block for this query?
		bool useTransaction;
	};

	//Query worker pool settings.
	struct PoolSettings
	{
	public:

		//Thread loop wait mode. Determines which method threads use to defer processing to other threads when in a wait loop.
		enum WaitMode { YIELD, SLEEP };
		
		/**
		* Thread process priority.
		* Part of Windows processthreadsapi.h
		* See https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setpriorityclass
		* 0x00000020 = "Normal priority"
		* 0x00004000 = "Below normal priority"
		* 0x00000040 = "Idle priority"
		* Other thread priority levels are not recommended for use.
		*/
		enum Priority { NORMAL = 0x00000020, BELOW_NORMAL = 0x00004000, IDLE = 0x00000040 };

		PoolSettings() :
			minPoolSize(1),
			maxPoolSize(8),
			waitMode(SLEEP),
			managerPriority(NORMAL),
			workerPriority(NORMAL)
		{};
		~PoolSettings() {};

		int minPoolSize;
		int maxPoolSize;
		WaitMode waitMode;
		Priority managerPriority;
		Priority workerPriority;
	};

	//A query object.
	struct PLYQuery
	{
		PLYQuery() :
			workerID(0),
			queryID(0),
			queryString(""),
			finished(false)
		{
			AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
			creationTime = AZ::ScriptTimePoint(now);
		};
		~PLYQuery() {};
		unsigned long long workerID;
		unsigned long long queryID;
		AZStd::string queryString;
		AZ::ScriptTimePoint creationTime;	
		QuerySettings settings;
		std::atomic<bool> finished;
	};

	//A query results object.
	struct PLYResult
	{
		enum ResultErrorType { NONE = 0, SQL_ERROR = 1, TTL_EXPIRED = 2 };

		PLYResult() :
			queryID(0),
			hasBeenAdvertised(false),
			errorType(ResultErrorType::NONE),
			errorMessage("")
		{
			AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
			resultCreationTime = AZ::ScriptTimePoint(now);
		};
		~PLYResult() {};
		unsigned long long queryID;
		pqxx::result resultSet;
		//Has the result been advertised via the query results bus?
		bool hasBeenAdvertised;
		AZ::ScriptTimePoint queryCreationTime;
		AZ::ScriptTimePoint queryStartTime;
		AZ::ScriptTimePoint queryEndTime;
		AZ::ScriptTimePoint resultCreationTime;
		QuerySettings settings;
		ResultErrorType errorType;
		AZStd::string errorMessage;
	};
}