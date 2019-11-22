// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include "Console.h"

#include <algorithm>
#include <cctype>
#include <string>

#include <PLYLog.h>

#include <PLY/PLYRequestBus.h>

#include <StatsCollector.h>

#include <ISystem.h>
#include <IConsole.h>

#include <platform_impl.h>

using namespace PLY;

Console::Console()
{
	//Register console commands.
	ISystem* system = nullptr;
	CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);
	if (system)
	{
		IConsole* console = system->GetIConsole();
		console->AddCommand("ply", &ConsoleCommand);
	}
	else
	{
		PLYLOG(PLYLog::PLY_ERROR, "Unable to get CrySystem pointer when registering console commands.");
	}
}

Console::~Console()
{

}

void Console::ConsoleCommand(IConsoleCmdArgs *cmdArgs)
{
	int argCount = cmdArgs->GetArgCount();
	if (argCount > 1)
	{
		const char* command1 = cmdArgs->GetArg(1);
		AZStd::string c1 = AZStd::string(command1);

		//Convert argument to lowercase
		std::transform(c1.begin(), c1.end(), c1.begin(),
			[](unsigned char c) { return std::tolower(c); });

		if (c1 == "set")
		{
			if (argCount > 2)
			{
				const char* command2 = cmdArgs->GetArg(2);
				AZStd::string c2 = AZStd::string(command2);

				//Convert argument to lowercase
				std::transform(c2.begin(), c2.end(), c2.begin(),
					[](unsigned char c) { return std::tolower(c); });

				if (c2 == "passes")
				{
					if (argCount > 3)
					{

						const char* command3 = cmdArgs->GetArg(3);
						AZStd::string c3 = AZStd::string(command3);

						//Convert argument to lowercase
						std::transform(c3.begin(), c3.end(), c3.begin(),
							[](unsigned char c) { return std::tolower(c); });
						
						int passes = 0;

						try
						{
							passes = std::stoi(c3.c_str());
						}
						catch (const std::invalid_argument& ia)
						{
							//Use variable to avoid compiler warning.
							ia.what();
							AZ_Printf("PLY", "%s", "Argument after passes must be an integer");
						}

						if (passes > 0)
						{
							AZ_Printf("PLY", "%s", ("Benchmark passes set to " + std::to_string(passes)).c_str());
							PLYRequestBus::Broadcast(&PLYRequestBus::Events::SetBenchmarkPasses, passes);
						}
						else
						{
							AZ_Printf("PLY", "%s", "Passes must be more than zero");
						}
					}
					else
					{
						AZ_Printf("PLY", "%s", "Passes requires an additional command");
					}
				}
				else if(c2 == "stats_interval")
				{
					const char* command3 = cmdArgs->GetArg(3);
					AZStd::string c3 = AZStd::string(command3);

					//Convert argument to lowercase
					std::transform(c3.begin(), c3.end(), c3.begin(),
						[](unsigned char c) { return std::tolower(c); });

					int interval = 0;

					try
					{
						interval = std::stoi(c3.c_str());
					}
					catch (const std::invalid_argument& ia)
					{
						//Use variable to avoid compiler warning.
						ia.what();
						AZ_Printf("PLY", "%s", "Argument after stats_interval must be an integer");
					}

					if (interval > 0)
					{
						AZ_Printf("PLY", "%s", ("Benchmark statistics display interval set to " + std::to_string(interval) + " seconds").c_str());
						STATS->SetInterval(interval);
					}
					else
					{
						AZ_Printf("PLY", "%s", "Passes must be more than zero");
					}
				}
				else
				{
					AZ_Printf("PLY", "%s", "Unknown set command");
				}
			}
			else
			{
				AZ_Printf("PLY", "%s", "Set requires an additional command");
			}
		}
		else if (c1 == "benchmark")
		{
			if (argCount > 2)
			{
				const char* command2 = cmdArgs->GetArg(2);
				AZStd::string c2 = AZStd::string(command2);

				//Convert argument to lowercase
				std::transform(c2.begin(), c2.end(), c2.begin(),
					[](unsigned char c) { return std::tolower(c); });

				if (c2 == "start")
				{
					if (argCount > 3)
					{

						const char* command3 = cmdArgs->GetArg(3);
						AZStd::string c3 = AZStd::string(command3);

						//Convert argument to lowercase
						std::transform(c3.begin(), c3.end(), c3.begin(),
							[](unsigned char c) { return std::tolower(c); });

						if (c3 == "simple")
						{
							AZ_Printf("PLY", "%s", "Starting benchmark on Simple Dataset");
							PLYRequestBus::Broadcast(&PLYRequestBus::Events::StartBenchmarkSimple);
						}
						else if (c3 == "stars")
						{
							AZ_Printf("PLY", "%s", "Starting benchmark on Stars Dataset");
							PLYRequestBus::Broadcast(&PLYRequestBus::Events::StartBenchmarkStars);
						}
						else if (c3 == "stars_sequence")
						{
							AZ_Printf("PLY", "%s", "Starting benchmark SEQUENCE on Stars Dataset");
							PLYRequestBus::Broadcast(&PLYRequestBus::Events::StartBenchmarkStarsSequence);
						}
						else
						{
							AZ_Printf("PLY", "%s", "Unknown benchmark start command");
						}
					}
					else
					{
						AZ_Printf("PLY", "%s", "Benchmark start requires an additional command");
					}
				}
				else if (c2 == "stop")
				{
					AZ_Printf("PLY", "%s", "Stopping Benchmark");
					PLYRequestBus::Broadcast(&PLYRequestBus::Events::StopBenchmark);
				}
				else
				{
					AZ_Printf("PLY", "%s", "Unknown benchmark command");
				}
			}
			else
			{
				AZ_Printf("PLY", "%s", "Benchmark requires an additional command");
			}
		}
		else if (c1 == "stats")
		{
			if (argCount > 2)
			{
				const char* command2 = cmdArgs->GetArg(2);
				AZStd::string c2 = AZStd::string(command2);

				//Convert argument to lowercase
				std::transform(c2.begin(), c2.end(), c2.begin(),
					[](unsigned char c) { return std::tolower(c); });

				if (c2 == "start")
				{
					AZ_Printf("PLY", "%s", "Starting Statistics Display");
					STATS->Start();
				}
				else if (c2 == "stop")
				{
					AZ_Printf("PLY", "%s", "Stopping Statistics Display");
					STATS->Stop();
				}
				else
				{
					AZ_Printf("PLY", "%s", "Unknown stats command");
				}
			}
			else
			{
				AZ_Printf("PLY", "%s", "Stats requires an additional command");
			}
		}
		else
		{
			AZ_Printf("PLY", "%s", "Unknown PLY command");
		}
	}
	else
	{
		PLYLOG(PLY::PLYLog::PLY_ERROR, "PLY requires additional commands");
	}
}