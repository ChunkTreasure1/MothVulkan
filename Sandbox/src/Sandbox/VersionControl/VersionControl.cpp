#include "sbpch.h"
#include "VersionControl.h"

#include "P4Implementation.h"

#include <Lamp/Log/Log.h>

void VersionControl::Initialize(VersionControlSystem system)
{
	switch (system)
	{
		case VersionControlSystem::Perforce:
			s_implementation = CreateScope<P4Implementation>();
			break;
	
		default:
			LP_CORE_ERROR("Invalid version control selected!");
			return;
			break;
	}

	s_implementation->InitializeImpl();
}

void VersionControl::Shutdown()
{
	s_implementation->ShutdownImpl();
	s_implementation = nullptr;
}

void VersionControl::Connect(const std::string& host, const std::string& port, const std::string& user, const std::string& password)
{
	s_implementation->ConnectImpl(host, port, user, password);
}

void VersionControl::Disconnect()
{
	s_implementation->DisconnectImpl();
}

void VersionControl::Add(const std::filesystem::path& file)
{
	s_implementation->AddImpl(file);
}

void VersionControl::Delete(const std::filesystem::path& file)
{
	s_implementation->DeleteImpl(file);
}

void VersionControl::Submit(const std::string& message)
{
	s_implementation->SubmitImpl(message);
}

void VersionControl::Sync(const std::string& depo)
{
	s_implementation->SyncImpl(depo);
}
