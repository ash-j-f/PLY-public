// PLY Gem console commands.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

//Forward declarations.
struct IConsoleCmdArgs;

namespace PLY
{
	class Console
	{
	public:
		Console();
		~Console();

		//Execute the given console command.
		static void ConsoleCommand(IConsoleCmdArgs* cmdArgs);
	};
}