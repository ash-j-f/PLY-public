// A simple console log class. Messages are printed to the Lumberyard game console.
// Derives from the Log class included with the PLY Gem.
// Designed to be used as a singleton, called using the provided macros.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <PLY/Log.hpp>

#define PLYLOG(level, message) PLY::PLYLog::getInstance()->Print(level, message)
#define PLYLOG_SET_LEVEL(level) PLY::PLYLog::getInstance()->SetLogLevel(level)
#define PLYLOG_GET_LEVEL PLY::PLYLog::getInstance()->GetLogLevel()
#define PLYLOG_GET_DEFAULT_LEVEL PLY::PLYLog::getInstance()->GetDefaultLogLevel()

namespace PLY
{
	class PLYLog : public Log
	{
	public:
		
		//Get the singleton instance.
		static PLYLog *getInstance();

		//Print a message to the console.
		//@param level The log level.
		//@param message The message to print.
		void Print(LogLevel level, AZStd::string message);

	private:

		PLYLog();
		~PLYLog();
	};
}