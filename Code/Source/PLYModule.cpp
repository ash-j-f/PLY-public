// Core Lumberyard module for the PLY Gem. Loads all required components.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <PLYSystemComponent.h>
#include <Components/PLYConfigurationComponent.h>
#include <Components/PLYObjectSyncComponent.h>

namespace PLY
{
    class PLYModule
        : public AZ::Module
    {
	public:

        AZ_RTTI(PLYModule, "{9C59EE4B-CC8E-4171-A87C-15A188C6BED8}", AZ::Module);
        AZ_CLASS_ALLOCATOR(PLYModule, AZ::SystemAllocator, 0);

        PLYModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                PLYSystemComponent::CreateDescriptor(),
				PLYConfigurationComponent::CreateDescriptor(),
				PLYObjectSyncComponent::CreateDescriptor()
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<PLYSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(PLY_8e5b5e01df344dc78dd024db9793b58b, PLY::PLYModule)
