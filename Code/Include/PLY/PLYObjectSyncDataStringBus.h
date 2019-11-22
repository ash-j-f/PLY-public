// Component bus for serialisation methods for automatic object and database synchronisation.
// This component bus is used to communicate with the custom serialisation and de-serialisation implmentation on each object
// to be synchronised.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <AzCore/EBus/EBus.h>

namespace PLY
{
	class PLYObjectSyncDataString
		: public AZ::ComponentBus
	{
	public:
		
		//Update the object's state from data in a given JSON string.
		//@param dataString The JSON string to parse.
		virtual void SetPropertiesFromDataString(std::string dataString) = 0;

		//Create a JSON String from the object's state.
		virtual std::string GetDataString() = 0;

		//Set the object invisible in the scene.
		virtual void SetObjectInvisible() = 0;

		//Set the object visible in the scene.
		virtual void SetObjectVisible() = 0;

		//Reset the object to its starting state.
		virtual void Reset() = 0;
	};
	using PLYObjectSyncDataStringBus = AZ::EBus<PLYObjectSyncDataString>;
} // namespace PLY
