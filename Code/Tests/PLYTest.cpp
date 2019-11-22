// A simple test to check if the PLY Gem is responding to API requests.
// @author Ashley Flynn - https://ajflynn.io/ - The Academy of Interactive Entertainment and the Canberra Institute of Technology - 2019

#include <AzTest/AzTest.h>

#include <PLY/PLYTools.h>

#include "PLYSystemComponent.h"

class PLYTest
    : public ::testing::Test
{

protected:

	PLY::PLYSystemComponent *TestComponent;

    void SetUp() override
    {
		TestComponent = new PLY::PLYSystemComponent();
    }

    void TearDown() override
    {
		delete TestComponent;
    }
};

/**
* Check if Libpq.dll (DLL dependency of Libpqxx static library) has been compiled thread safe.
* This also tests if the Libpqxx static library can be called, and if it can find and communicate with libpq.dll.
* By default, the copy of libpq.dll included with PostgreSQL install is thread safe.
* Essential DLL libraries must be present in the directory where the test EXE is executed from. See PLY install documentation.
*/
TEST_F(PLYTest, LibpqThreadSafe)
{
	bool result = TestComponent->GetLibpqThreadsafe();
	ASSERT_TRUE(result);
}

/**
* Check if a database connection can be established.
* A test database must already be created, called "libpqxxtest" on localhost port 5432, with user "test" and password "test".
* Essential DLL libraries must be present in the directory where the test EXE is executed from. See PLY install documentation.
*/
TEST_F(PLYTest, ConnectionTest)
{
	pqxx::connection c("postgresql://test:test@localhost/libpqxxtest");
	ASSERT_TRUE(c.protocol_version() > 0);
}

AZ_UNIT_TEST_HOOK();
