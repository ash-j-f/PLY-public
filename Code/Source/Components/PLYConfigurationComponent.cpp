// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include <regex>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Components/PLYConfigurationComponent.h>

#include <PLY/PLYRequestBus.h>

#include <PLY/PLYConfiguration.hpp>

using namespace PLY;

PLY::PLYConfigurationComponent::PLYConfigurationComponent()
{
	//Initialise with internal defaults.
	//These will be overridden when the component loads its settings from Lumberyard.

	m_logLevel = PLYLOG_GET_DEFAULT_LEVEL;

	DatabaseConnectionDetails d;

	m_port = d.port;
	m_host = d.host;
	m_database = d.database;
	m_username = d.username;
	m_password = d.password;
	m_reconnectWaitTime = d.reconnectWaitTime;
	m_connectionTimeout = d.connectTimeout;
	m_sslMode = d.sslMode;

	PoolSettings p;

	m_minPoolSize = p.minPoolSize;
	m_maxPoolSize = p.maxPoolSize;
	m_waitMode = p.waitMode;
	m_managerPriority = p.managerPriority;
	m_workerPriority = p.workerPriority;

}

PLY::PLYConfigurationComponent::~PLYConfigurationComponent()
{
}

void PLY::PLYConfigurationComponent::Reflect(AZ::ReflectContext* context)
{
	AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
	if (serialize)
	{
		// Reflect the class fields that you want to serialize.
		// Base classes with serialized data should be listed as additional template
		// arguments to the Class< T, ... >() function.
		serialize->Class<PLYConfigurationComponent, AZ::Component>()
			->Version(1)
			->Field("LogLevel", &PLYConfigurationComponent::m_logLevel)
			->Field("Port", &PLYConfigurationComponent::m_port)
			->Field("Host", &PLYConfigurationComponent::m_host)
			->Field("Database", &PLYConfigurationComponent::m_database)
			->Field("Username", &PLYConfigurationComponent::m_username)
			->Field("Password", &PLYConfigurationComponent::m_password)
			->Field("ReconnectWaitTime", &PLYConfigurationComponent::m_reconnectWaitTime)
			->Field("ConnectionTimeout", &PLYConfigurationComponent::m_connectionTimeout)
			->Field("SSLMode", &PLYConfigurationComponent::m_sslMode)
			->Field("MinPoolSize", &PLYConfigurationComponent::m_minPoolSize)
			->Field("MaxPoolSize", &PLYConfigurationComponent::m_maxPoolSize)
			->Field("ThreadWaitMode", &PLYConfigurationComponent::m_waitMode)
			->Field("ThreadManagerPriority", &PLYConfigurationComponent::m_managerPriority)
			->Field("ThreadWorkerPriority", &PLYConfigurationComponent::m_workerPriority)
			;

		AZ::EditContext* edit = serialize->GetEditContext();
		if (edit)
		{
			edit->Class<PLYConfigurationComponent>("PLY Configuration", "Configuration for the PLY PostgreSQL connection gem")
				->ClassElement(AZ::Edit::ClassElements::EditorData, "")
				->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
				->DataElement(AZ::Edit::UIHandlers::ComboBox, &PLYConfigurationComponent::m_logLevel,
					"Log Level", "Log message level")
				->EnumAttribute(PLY::PLYLog::LogLevel::PLY_ERROR, "Error")
				->EnumAttribute(PLY::PLYLog::LogLevel::PLY_WARNING, "Warning")
				->EnumAttribute(PLY::PLYLog::LogLevel::PLY_INFO, "Info")
				->EnumAttribute(PLY::PLYLog::LogLevel::PLY_DEBUG, "Debug")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYConfigurationComponent::m_port,
					"Port", "The database connection port")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->Attribute(AZ::Edit::Attributes::Min, 1)
				->Attribute(AZ::Edit::Attributes::Max, 65535)
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYConfigurationComponent::m_host,
					"Host", "The database connection host name")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYConfigurationComponent::m_database,
					"Database", "The database name")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYConfigurationComponent::m_username,
					"Username", "The database user name")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYConfigurationComponent::m_password,
					"Password", "The database user password")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYConfigurationComponent::m_reconnectWaitTime,
					"Reconnect Wait Time (ms)", "Time to wait in milliseconds before trying to reconnect to database")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->Attribute(AZ::Edit::Attributes::Min, 1)
				->Attribute(AZ::Edit::Attributes::Max, 300000)
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYConfigurationComponent::m_connectionTimeout,
					"Connection Timeout (s)", "Time to wait in seconds for a connection attempt to complete")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->Attribute(AZ::Edit::Attributes::Min, 0)
				->Attribute(AZ::Edit::Attributes::Max, 300)
				->DataElement(AZ::Edit::UIHandlers::ComboBox, &PLYConfigurationComponent::m_sslMode,
					"SSL Mode", "SSL connection mode")
				->EnumAttribute(DatabaseConnectionDetails::SSLMode::ALLOW, "Allow")
				->EnumAttribute(DatabaseConnectionDetails::SSLMode::DISABLE, "Disable")
				->EnumAttribute(DatabaseConnectionDetails::SSLMode::PREFER, "Prefer")
				->EnumAttribute(DatabaseConnectionDetails::SSLMode::REQUIRE, "Require")
				->EnumAttribute(DatabaseConnectionDetails::SSLMode::VERIFY_CA, "Verify CA")
				->EnumAttribute(DatabaseConnectionDetails::SSLMode::VERIFY_FULL, "Verify Full")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)

				->DataElement(AZ::Edit::UIHandlers::Default, &PLYConfigurationComponent::m_minPoolSize,
					"Min Pool Size", "The minimum connection pool size")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->Attribute(AZ::Edit::Attributes::Min, 1)
				->Attribute(AZ::Edit::Attributes::Max, 2048)
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYConfigurationComponent::m_maxPoolSize,
					"Max Pool Size", "The maximum connection pool size")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->Attribute(AZ::Edit::Attributes::Min, 1)
				->Attribute(AZ::Edit::Attributes::Max, 2048)
				->DataElement(AZ::Edit::UIHandlers::ComboBox, &PLYConfigurationComponent::m_waitMode,
					"Thread Wait Mode", "Thread loop wait mode")
				->EnumAttribute(PoolSettings::SLEEP, "Sleep")
				->EnumAttribute(PoolSettings::YIELD, "Yield")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->DataElement(AZ::Edit::UIHandlers::ComboBox, &PLYConfigurationComponent::m_managerPriority,
					"Manager Thread Priority", "Priority for the manager thread")
				->EnumAttribute(PoolSettings::NORMAL, "Normal")
				->EnumAttribute(PoolSettings::BELOW_NORMAL, "Below Normal")
				->EnumAttribute(PoolSettings::IDLE, "Idle")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				->DataElement(AZ::Edit::UIHandlers::ComboBox, &PLYConfigurationComponent::m_workerPriority,
					"Worker Thread Priority", "Priority for the worker thread")
				->EnumAttribute(PoolSettings::NORMAL, "Normal")
				->EnumAttribute(PoolSettings::BELOW_NORMAL, "Below Normal")
				->EnumAttribute(PoolSettings::IDLE, "Idle")
				->Attribute(AZ::Edit::Attributes::ChangeNotify, &PLYConfigurationComponent::SendConfigChanges)
				;
		}
	}
}

void PLY::PLYConfigurationComponent::Init()
{
	PLYLOG(PLYLog::PLY_DEBUG, "PLY configurator has initialised");
}

void PLY::PLYConfigurationComponent::Activate()
{

	PLYLOG(PLYLog::PLY_DEBUG, "PLY configurator has activated");

	//Send config options immediately on startup.
	SendConfigChanges();

}

void PLY::PLYConfigurationComponent::Deactivate()
{
	PLYLOG(PLYLog::PLY_DEBUG, "PLY configurator has deactivated");

}

void PLY::PLYConfigurationComponent::SendConfigChanges() const
{

	PLYCONF->SetLogLevel(m_logLevel);

	DatabaseConnectionDetails d;

	d.port = m_port;
	d.host = m_host;
	d.database = m_database;
	d.username = m_username;
	d.password = m_password;
	d.connectTimeout = m_connectionTimeout;
	d.sslMode = m_sslMode;

	PLYCONF->SetDatabaseConnectionDetails(d);

	PoolSettings p;

	p.minPoolSize = m_minPoolSize;
	p.maxPoolSize = m_maxPoolSize;
	p.waitMode = m_waitMode;
	p.managerPriority = m_managerPriority;
	p.workerPriority = m_workerPriority;

	PLYCONF->SetPoolSettings(p);
}
