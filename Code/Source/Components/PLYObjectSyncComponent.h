// A game entity component that provides core methods for synchronising the object with the database.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once
#include <AzCore/Component/Component.h>

#include <AzCore/Component/TickBus.h>

#include <PLY/PLYObjectSyncSaveLoadBus.h>
#include <PLY/PLYObjectSyncDataStringBus.h>
#include <PLY/PLYObjectSyncEntitiesBus.h>

#include <PLY/PLYResultBus.h>

#include <PLY/PLYTools.h>

namespace PLY
{

	class PLYObjectSyncComponent
		: public AZ::Component,
		public AZ::TickBus::Handler,
		public PLY::PLYResultBus::Handler,
		public PLY::PLYObjectSyncSaveLoadBus::Handler,
		public PLY::PLYObjectSyncEntitiesBus::Handler
	{

	public:

		PLYObjectSyncComponent();
		~PLYObjectSyncComponent();

		//UUID crested using VS > Tools > Create GUID. This can be any GUID unique to the project.
		AZ_COMPONENT(PLYObjectSyncComponent, "{5ACC041D-75A1-47D0-876C-F33E498C566E}", AZ::Component);

		//Required Reflect function.
		static void Reflect(AZ::ReflectContext* context);

		//When to automatically sync object with database.
		//All options automatically load object data from database on object construction, except option NEVER.
		enum AutomaticUpdateFrequency
		{
			NEVER, //Only sync when explicitly called.
			USER_FEFINED //Sync at a user defined time interval.
		};

	protected:

		// AZ::Component interface implementation.
		void Init() override;
		void Activate() override;
		void Deactivate() override;

		//Tick handler.
		void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

		//Tick order for tick handler.
		inline int GetTickOrder() override { return AZ::ComponentTickBus::TICK_GAME; };

		//Save object state to database.
		void Save() override;
		
		//Save object state to database, using the provided data string.
		//@param dataString The data string to save to the database.
		void Save(std::string dataString) override;
		
		//Load object state from the database.
		void Load() override;
		
		//Get the object's unique database ID.
		inline int GetObjectID() override { return m_objectID; };
		
		//Set the object's unique database ID.
		void SetObjectID(int newID) override;

		//Get the unique database IDs for all objects with automatic database sync capability.
		inline int GetAllObjectIDs() override { return m_objectID; };
		
		//Get the Lumberyard entity IDs for all objects with automatic database sync capability.
		inline AZ::EntityId GetAllEntityIDs() override { return GetEntityId(); };
		
		//Get pairs of Lumberyard entity ID and Lumberyard entity ID for all objects with automatic database sync capability.
		inline std::pair<AZ::EntityId, int> GetAllEntityIDandObjectIDs() override { return std::pair<AZ::EntityId, int>(GetEntityId(), m_objectID); };
		
		//Get the database configuration details set on this entity.
		inline DataBaseDetails GetDatabaseDetails() override { DataBaseDetails d(m_tableName, m_IDColumnName, m_dataColumnName); return d; };

		//Reset the state of all objects with automatic database sync capability.
		void Reset() override;

		// Optional functions for defining provided and dependent services.
		/*
		static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
		static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
		static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
		static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
		*/

		//Has the object loaded data on construction?
		bool m_hasLoaded;

		//Is the loading process in progress?
		bool m_isLoading;

		//Unique object ID.
		int m_objectID;

		//Should the object sync with database on load?
		bool m_syncOnLoad;

		//Has the entity been set visible once?
		bool m_setVisibleOnce;

		//Has the object been set invisible once?
		bool m_setInvisibleOnce;

		//Table and column names to use for object data storage and retrieval.
		AZStd::string m_tableName; //The table in which object data is stored.
		AZStd::string m_IDColumnName; //Unique ID column. Must be an INT and the PRIMARY KEY so as not to allow duplicates.
		AZStd::string m_dataColumnName; //The column in which serialised JSON data for the object is stored. Must be of TEXT type.

		//Mutex to lock the data string while it is being changed.
		std::mutex m_dataStringMutex;

		//Data string that represents the state of this object.
		std::string m_dataString;

		//Mutex to lock the load query list while it being changed.
		std::mutex m_queryIDsLoadMutex;
		//Query IDs of queries sent to the database to load data for this object.
		std::vector<unsigned long long> m_queryIDsLoad;

		//Mutex to lock the save query list while it is being changed.
		std::mutex m_queryIDsSaveMutex;
		//Query IDs of queries sent to the database to save data for this object.
		std::vector<unsigned long long> m_queryIDsSave;

		//When to automatically sync object with database.
		AutomaticUpdateFrequency m_updateFrequencyMode;
		
		//User-defined object sync frequency, in milliseconds. Only applies if m_updateFrequency is set to USER_DEFINED.
		int m_userDefinedFrequencyMS; //Milliseconds. 

		//Time since object was last synced with the database.
		float m_timer;

		//Advertises a result ID is ready.
		//@param queryID The ID of the ready result.
		void ResultReady(const unsigned long long queryID) override;
	};
}