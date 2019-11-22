// PLY configuration component, that manages PLY initialisation in a scene, with config options exposed to the editor.
// Only one PLY configuration component should be present in a scene.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <AzCore/Component/Component.h>
#include <PLY/PLYTools.h>
#include <PLY/PLYTypes.h>
#include <PLYLog.h>

namespace PLY
{

	class PLYConfigurationComponent
		: public AZ::Component
	{

	public:

		PLYConfigurationComponent();
		~PLYConfigurationComponent();

		//UUID crested using VS > Tools > Create GUID. This can be any GUID unique to the project.
		AZ_COMPONENT(PLYConfigurationComponent, "{1700A1E0-5AE7-49AB-B1F1-DEE5286E5130}", AZ::Component);

		//Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

	protected:

		//Log level.
		PLYLog::LogLevel m_logLevel;

		//Database connection details.
		int m_port;
		AZStd::string m_host;
		AZStd::string m_database;
		AZStd::string m_username;
		AZStd::string m_password;
		int m_reconnectWaitTime;
		int m_connectionTimeout;
		DatabaseConnectionDetails::SSLMode m_sslMode;

		//Query worker pool settings.
		int m_minPoolSize;
		int m_maxPoolSize;
		PoolSettings::WaitMode m_waitMode;
		PoolSettings::Priority m_managerPriority;
		PoolSettings::Priority m_workerPriority;

		//AZ::Component interface implementation.
		void Init() override;
		void Activate() override;
		void Deactivate() override;

		// Optional functions for defining provided and dependent services.
		/*
		static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
		static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
		static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
		static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
		*/

		//Send config changes to the PLY Configuration singleton.
		void SendConfigChanges() const;

	};
}