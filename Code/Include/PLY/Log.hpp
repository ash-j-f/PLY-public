// A simple logging class that outputs log messages to the Lumberyard Console.
// Self-contained as a HPP file so that it can be included and used by external projects.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <AzCore/std/string/string.h>

namespace PLY
{
	class Log
	{

	public:

		//Available log levels.
		enum LogLevel { PLY_ERROR = 0, PLY_WARNING = 1, PLY_INFO = 2, PLY_DEBUG = 3 };

		Log() : m_logLevel(m_defaultLogLevel) //Set default log level;
		{
		}

		virtual ~Log()
		{
		}

		//Print a message to the console.
		//@param level The log level.
		//@param message The message to print.
		void Print(LogLevel level, AZStd::string message) const
		{
			if (level <= m_logLevel)
			{
				AZStd::string levelName;
				switch (level)
				{
				case PLY_ERROR:
					levelName = "Error";
					break;
				case PLY_WARNING:
					levelName = "Warning";
					break;
				case PLY_INFO:
					levelName = "Info";
					break;
				case PLY_DEBUG:
					levelName = "Debug";
					break;
				default:
					Print(PLY_ERROR, "Unknown log level sent to Print function");
					break;
				}

				AZ_Printf(("PLY " + levelName).c_str(), "%s", message.c_str());
			}
		}

		//Set the log level.
		//@param level The new log level.
		void SetLogLevel(LogLevel level)
		{
			m_logLevel = level;
		}

		//Get the current log level.
		LogLevel GetLogLevel() const
		{
			return m_logLevel;
		}

		//Get the default log level.
		LogLevel GetDefaultLogLevel() const
		{
			return m_defaultLogLevel;
		}

	private:

		//Default log level on Log object creation.
		const LogLevel m_defaultLogLevel = PLY_ERROR;

		//Current log level for logging messages.
		LogLevel m_logLevel;

	};
}