// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include <regex>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <PLY/PLYTypes.h>
#include <PLYLog.h>
#include "PLYObjectSyncComponent.h"
#include <PLY/PLYRequestBus.h>

using namespace PLY;

PLY::PLYObjectSyncComponent::PLYObjectSyncComponent() :
	m_dataString(""),
	m_hasLoaded(false),
	m_isLoading(false),
	m_syncOnLoad(true),
	m_setInvisibleOnce(false),
	m_setVisibleOnce(false),
	m_objectID(0),
	m_updateFrequencyMode(AutomaticUpdateFrequency::NEVER),
	m_userDefinedFrequencyMS(1000), //Milliseconds
	m_timer(0),
	m_tableName(""),
	m_IDColumnName(""),
	m_dataColumnName("")
{
	

}

PLY::PLYObjectSyncComponent::~PLYObjectSyncComponent()
{
}

void PLY::PLYObjectSyncComponent::Reflect(AZ::ReflectContext* context)
{
	AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
	if (serialize)
	{
		// Reflect the class fields that you want to serialize.
		// Base classes with serialized data should be listed as additional template
		// arguments to the Class< T, ... >() function.
		serialize->Class<PLYObjectSyncComponent, AZ::Component>()
			->Version(1)
			->Field("ObjectID", &PLYObjectSyncComponent::m_objectID)
			->Field("TableName", &PLYObjectSyncComponent::m_tableName)
			->Field("IDColumnName", &PLYObjectSyncComponent::m_IDColumnName)
			->Field("DataColumnName", &PLYObjectSyncComponent::m_dataColumnName)
			->Field("UpdateFrequency", &PLYObjectSyncComponent::m_updateFrequencyMode)
			->Field("UserUpdateFrequency", &PLYObjectSyncComponent::m_userDefinedFrequencyMS)
			->Field("SyncOnLoad", &PLYObjectSyncComponent::m_syncOnLoad)
			;

		AZ::EditContext* edit = serialize->GetEditContext();
		if (edit)
		{
			edit->Class<PLYObjectSyncComponent>("PLY Object Database Sync", "Automatically synchronise an object with the datase")
				->ClassElement(AZ::Edit::ClassElements::EditorData, "")
				->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYObjectSyncComponent::m_objectID, "Object ID", "The unique object ID")
					->Attribute(AZ::Edit::Attributes::Min, 0)
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYObjectSyncComponent::m_tableName, "Table Name", "Storage table name")
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYObjectSyncComponent::m_IDColumnName, "ID Column Name", "Object identifier column name")
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYObjectSyncComponent::m_dataColumnName, "Data Column Name", "Object storage column name")
				->DataElement(AZ::Edit::UIHandlers::ComboBox, &PLYObjectSyncComponent::m_updateFrequencyMode,
					"Update Frequency", "Automatic Update Frequency")
					->EnumAttribute(AutomaticUpdateFrequency::NEVER, "Never")
					->EnumAttribute(AutomaticUpdateFrequency::USER_FEFINED, "User Defined")
				->DataElement(AZ::Edit::UIHandlers::Default, &PLYObjectSyncComponent::m_userDefinedFrequencyMS, 
					"Frequency (ms)", "User defined update frequency")
					->Attribute(AZ::Edit::Attributes::Min, 0)
				->DataElement(AZ::Edit::UIHandlers::CheckBox, &PLYObjectSyncComponent::m_syncOnLoad, "Sync On Load", "Load object data from database on start")
				;
		}
	}
}

void PLY::PLYObjectSyncComponent::Init()
{
	
}

void PLY::PLYObjectSyncComponent::Activate()
{
	AZ::TickBus::Handler::BusConnect();
	PLYResultBus::Handler::BusConnect();
	PLYObjectSyncSaveLoadBus::Handler::BusConnect(GetEntityId());
	PLYObjectSyncEntitiesBus::Handler::BusConnect();
}

void PLY::PLYObjectSyncComponent::Deactivate()
{
	PLYObjectSyncSaveLoadBus::Handler::BusDisconnect();
	PLYResultBus::Handler::BusDisconnect();
	AZ::TickBus::Handler::BusDisconnect();
	PLYObjectSyncEntitiesBus::Handler::BusDisconnect();
}

void PLY::PLYObjectSyncComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
{
	//Set object invisible on startup.
	if (!m_setInvisibleOnce)
	{
		m_setInvisibleOnce = true;

		//Set object invisible.
		PLYObjectSyncDataStringBus::Event(GetEntityId(), &PLYObjectSyncDataStringBus::Events::SetObjectInvisible);
	}

	//Automatically load object data on first frame.
	if (m_syncOnLoad && !m_isLoading && !m_hasLoaded && m_updateFrequencyMode != NEVER)
	{
		Load();
		m_isLoading = true;
	}
	
	//If sync on start is not selected, always assume initial object data is loaded.
	if (!m_syncOnLoad)
	{
		m_hasLoaded = true;

		if (!m_setVisibleOnce)
		{
			m_setVisibleOnce = true;
			//Set object visible.
			PLYObjectSyncDataStringBus::Event(GetEntityId(), &PLYObjectSyncDataStringBus::Events::SetObjectVisible);
		}
	}

	//Update object if datastring has value.
	std::unique_lock<std::mutex> lockDS(m_dataStringMutex);
	if (m_dataString != "")
	{
		//Broadcast datastring on bus to owning object
		PLYObjectSyncDataStringBus::Event(GetEntityId(), &PLYObjectSyncDataStringBus::Events::SetPropertiesFromDataString, m_dataString);

		m_dataString = "";

		//Set object visible.
		PLYObjectSyncDataStringBus::Event(GetEntityId(), &PLYObjectSyncDataStringBus::Events::SetObjectVisible);
	}

	//Automatically save object data at chosen interval, if requested.
	//Do not do this if object hasn't finished loading its initial data.
	if (m_hasLoaded && m_updateFrequencyMode == AutomaticUpdateFrequency::USER_FEFINED && m_userDefinedFrequencyMS != 0)
	{
		m_timer += deltaTime;

		//Convert milliseconds to seconds.
		float maxTime = static_cast<float>(m_userDefinedFrequencyMS) / 1000.0f;

		if (m_timer >= maxTime)
		{
			m_timer = 0;

			Save();
		}
	}
}

void PLY::PLYObjectSyncComponent::Save()
{
	//Broadcast on bus to request JSON string from object.
	std::string dataString = "";
	PLYObjectSyncDataStringBus::EventResult(dataString, GetEntityId(), &PLYObjectSyncDataStringBus::Events::GetDataString);

	AZ_Error("PLY", dataString != "", "Data string for object sync is blank. Does the object have a "
		"custom script attached that answers to requests on the Ebus \"PLYObjectSyncDataStringBus?\"");

	if (dataString != "") Save(dataString);
}

void PLY::PLYObjectSyncComponent::Save(std::string dataString)
{
	AZ_Error("PLY", m_objectID != 0, "Object ID is not set.");
	AZ_Error("PLY", m_tableName != "", "Object table name is not set.");
	AZ_Error("PLY", m_IDColumnName != "", "Object ID column name is not set.");
	AZ_Error("PLY", m_dataColumnName != "", "Object data column name is not set.");

	AZ_Error("PLY", dataString != "", "Data string for object sync is blank.");

	if (m_objectID == 0 || m_tableName == "" || m_IDColumnName == "" || m_dataColumnName == "" || dataString == "") return;

	//Save data to database using UPSERT.
	AZStd::string qString = ("insert into " + m_tableName + " (" + m_IDColumnName + ", "+ m_dataColumnName +
		") VALUES(" + std::to_string(m_objectID).c_str() + ", '" + dataString.c_str() +
		"') on conflict (" + m_IDColumnName + ") do update set " + m_dataColumnName + " = EXCLUDED." + m_dataColumnName).c_str();

	unsigned long long queryID = 0;

	PLY::PLYRequestBus::BroadcastResult(queryID, &PLY::PLYRequestBus::Events::SendQuery, qString);

	std::unique_lock<std::mutex> lockQ(m_queryIDsSaveMutex);
	if (queryID != 0) m_queryIDsSave.push_back(queryID);
	lockQ.unlock();
}

void PLY::PLYObjectSyncComponent::Load()
{
	AZ_Error("PLY", m_objectID != 0, "Object ID is not set.");
	AZ_Error("PLY", m_tableName != "", "Object table name is not set.");
	AZ_Error("PLY", m_IDColumnName != "", "Object ID column name is not set.");
	AZ_Error("PLY", m_dataColumnName != "", "Object data column name is not set.");

	if (m_objectID == 0 || m_tableName == "" || m_IDColumnName == "" || m_dataColumnName == "") return;

	unsigned long long queryID = 0;

	AZStd::string qString = ("select " + m_dataColumnName + " from " + m_tableName + " where " + 
		m_IDColumnName + " = " + std::to_string(m_objectID).c_str()).c_str();

	PLY::PLYRequestBus::BroadcastResult(queryID, &PLY::PLYRequestBus::Events::SendQuery, qString);

	std::unique_lock<std::mutex> lockQ(m_queryIDsLoadMutex);
	if (queryID != 0) m_queryIDsLoad.push_back(queryID);
	lockQ.unlock();
}

void PLY::PLYObjectSyncComponent::SetObjectID(int newID)
{
	AZ_Error("PLY", newID > 0, "Object ID must be greater than 0.");
	m_objectID = newID;
}

void PLY::PLYObjectSyncComponent::Reset()
{
	PLYObjectSyncDataStringBus::Event(GetEntityId(), &PLYObjectSyncDataStringBus::Events::Reset);
}

void PLY::PLYObjectSyncComponent::ResultReady(const unsigned long long queryID)
{
	std::unique_lock<std::mutex> lockL(m_queryIDsLoadMutex);
	bool isLoadQuery = (std::find(m_queryIDsLoad.begin(), m_queryIDsLoad.end(), queryID) != m_queryIDsLoad.end());
	lockL.unlock();

	if (isLoadQuery)
	{
		std::shared_ptr<PLY::PLYResult> result;

		//Check query completed ok.
		PLY::PLYRequestBus::BroadcastResult(result, &PLY::PLYRequestBus::Events::GetResult, queryID);
		if (result == nullptr || result->errorType != PLY::PLYResult::NONE || result->errorMessage != "")
		{
			PLYLOG(PLY::PLYLog::PLY_ERROR, "Object sync failed. Couldn't load data from the database due to a query error.");
			m_hasLoaded = false;
			m_isLoading = false;
		}
		else
		{
			//Even if result was blank, a successful load query means an attempt was made to load object data at least once.
			m_hasLoaded = true;
			m_isLoading = false;

			if (result->resultSet.size() > 0)
			{
				pqxx::row row = result->resultSet.front();
				pqxx::field field = row.front();
				const char *f = field.c_str();

				std::unique_lock<std::mutex> lockDS(m_dataStringMutex);
				m_dataString = f;
				lockDS.unlock();
			}
			else
			{
				//Data is blank, which is normal when the object has not been saved yet.

				//Set object visible.
				PLYObjectSyncDataStringBus::Event(GetEntityId(), &PLYObjectSyncDataStringBus::Events::SetObjectVisible);

			}
		}

		//Remove the query ID from our list.
		std::unique_lock<std::mutex> lockQ(m_queryIDsLoadMutex);
		m_queryIDsLoad.erase(std::remove(m_queryIDsLoad.begin(), m_queryIDsLoad.end(), queryID), m_queryIDsLoad.end());
		lockQ.unlock();
		PLY::PLYRequestBus::Broadcast(&PLY::PLYRequestBus::Events::RemoveResult, queryID);
	}

	std::unique_lock<std::mutex> lockS(m_queryIDsSaveMutex);
	bool isSaveQuery = (std::find(m_queryIDsSave.begin(), m_queryIDsSave.end(), queryID) != m_queryIDsSave.end());
	lockS.unlock();

	if (isSaveQuery)
	{
		std::shared_ptr<PLY::PLYResult> result;

		//Check query completed ok.
		PLY::PLYRequestBus::BroadcastResult(result, &PLY::PLYRequestBus::Events::GetResult, queryID);
		if (result == nullptr || result->errorType != PLY::PLYResult::NONE || result->errorMessage != "")
		{
			PLYLOG(PLY::PLYLog::PLY_ERROR, "Object sync failed. Couldn't save data to the database due to a query error.");
		}

		//Remove the query ID from our list.
		std::unique_lock<std::mutex> lockQ(m_queryIDsSaveMutex);
		m_queryIDsSave.erase(std::remove(m_queryIDsSave.begin(), m_queryIDsSave.end(), queryID), m_queryIDsSave.end());
		lockQ.unlock();
		PLY::PLYRequestBus::Broadcast(&PLY::PLYRequestBus::Events::RemoveResult, queryID);
	}
}
