// Configuration manager for the PLY Gem. Provides central storage and retrieval point for all Gem config options.
// Designed to be used as a singleton, called via the provided macros.
// Implemeted as a self-contained HPP file so it can be included and called from external projects.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <regex>

#include <PLY/PLYTools.h>
#include <PLY/PLYTypes.h>
#include <PLY/Log.hpp>

#define PLYCONF PLY::PLYConfiguration::getInstance()

namespace PLY
{
	class PLYConfiguration
	{
	public:

		//Get singleton instance.
		static PLYConfiguration *getInstance()
		{
			static PLYConfiguration instance;
			return &instance;
		};

		//Set database connection details.
		//@param d The database connection details.
		void SetDatabaseConnectionDetails(DatabaseConnectionDetails d) 
		{

			AZ_Error("PLY", d.connectTimeout >= 0, "Connection timeout cannot be less than 0");
			AZ_Error("PLY", d.reconnectWaitTime >= 0, "Reconnect wait time cannot be less than 0");
			AZ_Error("PLY", d.port >= 0, "Port number cannot be less than 0");
			
			std::unique_lock<std::mutex> lock(m_configMutex);
			m_d = d;

			//Update connection string from database connection settings.
			SetConnectionString();
		};

		//Set default query settings.
		//@param qs The default query settings.
		void SetQuerySettings(QuerySettings qs)
		{
			AZ_Error("PLY", qs.queryTTL >= 0, "Query TTL cannot be less than 0");
			AZ_Error("PLY", qs.resultTTL >= 0, "Result TTL cannot be less than 0");

			std::unique_lock<std::mutex> lock(m_configMutex);
			m_qs = qs;
		};

		//Set query worker pool settings.
		//@param p The query worker pool settings.
		void SetPoolSettings(PoolSettings p)
		{

			AZ_Error("PLY", p.minPoolSize >= 1, "Minimum pool size cannot be less than 1");
			AZ_Error("PLY", p.maxPoolSize >= 1, "Maximum pool size cannot be less than 1");
			
			if (p.minPoolSize > p.maxPoolSize)
			{
				AZ_Printf("PLY", "%s", "Minimum pool size must be less than or equal to maximum pool size. Swapping values.");

				std::swap(p.minPoolSize, p.maxPoolSize);
			}

			std::unique_lock<std::mutex> lock(m_configMutex);
			m_p = p;
		};

		//Set log level.
		//@param logLevel The chosen log level.
		void SetLogLevel(Log::LogLevel logLevel)
		{
			std::unique_lock<std::mutex> lock(m_configMutex);
			m_logLevel = logLevel;
		};

		//Get current database connection details.
		DatabaseConnectionDetails GetDatabaseConnectionDetails()
		{
			std::unique_lock<std::mutex> lock(m_configMutex);
			return m_d;
		};

		//Get current default query settings.
		QuerySettings GetQuerySettings()
		{
			std::unique_lock<std::mutex> lock(m_configMutex);
			return m_qs;
		};

		//Get current query worker pool settings.
		PoolSettings GetPoolSettings()
		{
			std::unique_lock<std::mutex> lock(m_configMutex);
			return m_p;
		};

		//Get current log level.
		Log::LogLevel GetLogLevel()
		{
			std::unique_lock<std::mutex> lock(m_configMutex);
			return m_logLevel;
		};

		//Get current database connection string.
		//Connection string is compatible with libpq connection strings.
		//See https://www.postgresql.org/docs/11/libpq-connect.html
		AZStd::string GetConnectionString()
		{
			std::unique_lock<std::mutex> lock(m_configMutex);
			return m_connectionString;
		};

	private:

		//Mutex used to lock the config class during changes.
		std::mutex m_configMutex;

		//Database connection detail settings.
		DatabaseConnectionDetails m_d;

		//Default query settings.
		QuerySettings m_qs;

		//Query worker pool settings.
		PoolSettings m_p;

		//Log level.
		Log::LogLevel m_logLevel;

		//The database connection string.
		//Connection string is compatible with libpq connection strings.
		//See https://www.postgresql.org/docs/11/libpq-connect.html
		AZStd::string m_connectionString;

		PLYConfiguration() : m_logLevel(Log::LogLevel::PLY_ERROR) {};
		~PLYConfiguration() {};

		//Set the internally stored database connection string based on current database connection detail settings.
		void SetConnectionString()
		{

			//Operate on a copy of the connection details struct for regex replacements.
			DatabaseConnectionDetails d = m_d;

			//Replace all \ characters with \\
			//Replace all ' characters with \'
			d.host = (std::regex_replace(m_d.host.c_str(), std::regex("\\\\"), "\\\\")).c_str();
			d.host = (std::regex_replace(m_d.host.c_str(), std::regex("'"), "\\'")).c_str();
			d.username = (std::regex_replace(m_d.username.c_str(), std::regex("\\\\"), "\\\\")).c_str();
			d.username = (std::regex_replace(m_d.username.c_str(), std::regex("'"), "\\'")).c_str();
			d.database = (std::regex_replace(m_d.database.c_str(), std::regex("\\\\"), "\\\\")).c_str();
			d.database = (std::regex_replace(m_d.database.c_str(), std::regex("'"), "\\'")).c_str();
			d.password = (std::regex_replace(m_d.password.c_str(), std::regex("\\\\"), "\\\\")).c_str();
			d.password = (std::regex_replace(m_d.password.c_str(), std::regex("'"), "\\'")).c_str();

			//Construct connection string.
			//Lock workers mutex so workers don't try to use constring as it it being written.
			AZStd::string conString = "host='" + d.host + "'";
			conString += " port='" + AZStd::string::format("%d", m_d.port) + "'";
			conString += " dbname='" + d.database + "'";
			conString += " user='" + d.username + "'";
			conString += " password='" + d.password + "'";

			//Only apply connection timeout setting if not set to 0. Otherwise Libpq will use its internal default.
			if (m_d.connectTimeout != 0) conString += " connect_timeout='" + AZStd::string::format("%d", m_d.connectTimeout) + "'";

			conString += " sslmode='";
			switch (m_d.sslMode)
			{
			case DatabaseConnectionDetails::SSLMode::ALLOW:
				conString += "allow";
				break;
			case DatabaseConnectionDetails::SSLMode::DISABLE:
				conString += "disable";
				break;
			case DatabaseConnectionDetails::SSLMode::PREFER:
				conString += "prefer";
				break;
			case DatabaseConnectionDetails::SSLMode::REQUIRE:
				conString += "require";
				break;
			case DatabaseConnectionDetails::SSLMode::VERIFY_CA:
				conString += "verify-ca";
				break;
			case DatabaseConnectionDetails::SSLMode::VERIFY_FULL:
				conString += "verify-full";
				break;
			default:
				AZ_Printf("PLY", "%s", "Unknown SSL Mode option sent to database connection string.");
				break;
			}
			conString += "'";

			m_connectionString = conString;
		};
	};
}