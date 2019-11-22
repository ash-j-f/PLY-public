// EBusTraits bus for communicating with all entities in the scene that are enabled for automatic object and database synchronisation.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <AzCore/EBus/EBus.h>

namespace PLY
{
	class PLYObjectSyncEntities
		: public AZ::EBusTraits
	{
	public:
		//////////////////////////////////////////////////////////////////////////
		// EBusTraits overrides
		static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
		static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
		//////////////////////////////////////////////////////////////////////////

		//Get the unique database IDs for all objects with automatic database sync capability.
		virtual int GetAllObjectIDs() = 0;

		//Get the Lumberyard entity IDs for all objects with automatic database sync capability.
		virtual AZ::EntityId GetAllEntityIDs() = 0;

		//Get pairs of Lumberyard entity ID and Lumberyard entity ID for all objects with automatic database sync capability.
		virtual std::pair<AZ::EntityId, int> GetAllEntityIDandObjectIDs() = 0;

		//Reset the state of all objects with automatic database sync capability.
		virtual void Reset() = 0;
	};
	using PLYObjectSyncEntitiesBus = AZ::EBus<PLYObjectSyncEntities>;
} // namespace PLY
