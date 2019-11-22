// PLY Gem query results EBusTraits ebus. Used by projects to receive event messages about completed query results.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <AzCore/EBus/EBus.h>

namespace PLY
{
    class PLYResults
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

		//Advertises a result ID is ready.
		//@param queryID The ID of the ready result.
		virtual void ResultReady(const unsigned long long queryID) = 0;
    };
    using PLYResultBus = AZ::EBus<PLYResults>;
} // namespace PLY
