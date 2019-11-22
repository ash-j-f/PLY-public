// Component bus for trigger save and load methods for entities enabled for automatic object and database synchronisation.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#pragma once

#include <AzCore/EBus/EBus.h>

namespace PLY
{
	class PLYObjectSyncSaveLoad
		: public AZ::ComponentBus
	{
	public:

		//Database configuration details.
		class DataBaseDetails
		{
		public:
			DataBaseDetails() {};
			DataBaseDetails(AZStd::string n_tableName, AZStd::string n_IDColumnName, AZStd::string n_dataColumnName) :
				tableName(n_tableName), IDColumnName(n_IDColumnName), dataColumnName(n_dataColumnName) {};
			~DataBaseDetails() {};

			//Table and column names to use for object data storage and retrieval.
			AZStd::string tableName; //The table in which object data is stored.
			AZStd::string IDColumnName; //Unique ID column. Must be an INT and the PRIMARY KEY so as not to allow duplicates.
			AZStd::string dataColumnName; //The column in which serialised JSON data for the object is stored. Must be of TEXT type.
		};

		//Save object state to database.
		virtual void Save() = 0;

		//Save object state to database, using the provided data string.
		//@param dataString The data string to save to the database.
		virtual void Save(std::string dataString) = 0;

		//Load object state from the database.
		virtual void Load() = 0;

		//Get the object's unique database ID.
		virtual int GetObjectID() = 0;

		//Set the object's unique database ID.
		virtual void SetObjectID(int newID) = 0;

		//Get the database configuration details set on this entity.
		virtual DataBaseDetails GetDatabaseDetails() = 0;
	};
	using PLYObjectSyncSaveLoadBus = AZ::EBus<PLYObjectSyncSaveLoad>;
} // namespace PLY
