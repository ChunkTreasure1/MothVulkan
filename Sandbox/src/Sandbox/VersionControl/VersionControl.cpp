#include "sbpch.h"
#include "VersionControl.h"

#include "Sandbox/VersionControl/P4/P4Implementation.h"

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
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->ShutdownImpl();
	s_implementation = nullptr;
}

bool VersionControl::Connect(const std::string& server, const std::string& user, const std::string& password)
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	return s_implementation->ConnectImpl(server, user, password);
}

void VersionControl::Disconnect()
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->DisconnectImpl();
}

void VersionControl::Add(const std::filesystem::path& file)
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->AddImpl(file);
}

void VersionControl::Delete(const std::filesystem::path& file)
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->DeleteImpl(file);
}

void VersionControl::Submit(const std::string& message)
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->SubmitImpl(message);
}

void VersionControl::Sync(const std::string& depo)
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->SyncImpl(depo);
}

void VersionControl::SwitchStream(const std::string& newStream)
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->SwitchStreamImpl(newStream);
}

void VersionControl::RefreshStreams()
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->RefreshStreamsImpl();
}

void VersionControl::SwitchWorkspace(const std::string& workspace)
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->SwitchWorkspaceImpl(workspace);
}

void VersionControl::RefreshWorkspaces()
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	s_implementation->RefreshWorkspacesImpl();
}

const std::vector<std::string>& VersionControl::GetWorkspaces()
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	return s_implementation->GetWorkspacesImpl();
}

const std::vector<std::string>& VersionControl::GetStreams()
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	return s_implementation->GetStreamsImpl();
}

bool VersionControl::IsConnected()
{
	LP_ASSERT(s_implementation, "No implementation loaded!");
	return s_implementation->IsConnectedImpl();
}
