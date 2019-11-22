// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include "PLYLog.h"

#include <PLY/PLYConfiguration.hpp>

using namespace PLY;

PLYLog::PLYLog()
{
	//Set log level on startup.
	SetLogLevel(PLYCONF->GetLogLevel());
}

PLYLog::~PLYLog()
{

}

PLYLog *PLYLog::getInstance()
{
	static PLYLog instance;
	return &instance;
}

void PLYLog::Print(LogLevel level, AZStd::string message)
{
	//Set log level from config class. 
	//This must be done here instead of in constrcutor as PLY config class may not have 
	//been updated from PLYConfigurationComponent at the time PLYLog was first instantiated. 
	PLYLOG_SET_LEVEL(PLYCONF->GetLogLevel());

	//Call base class Print.
	Log::Print(level, message);
}