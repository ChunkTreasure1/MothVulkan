#include "sbpch.h"
#include "P4Implementation.h"

#include <Lamp/Log/Log.h>

#include <filesystem>
#include <format>

void P4Implementation::InitializeImpl()
{
	Error e;
	P4Libraries::Initialize(P4LIBRARIES_INIT_ALL, &e);

	m_client.EnableExtensions(&e);
}

void P4Implementation::ShutdownImpl()
{
	Error e;
	P4Libraries::Shutdown(P4LIBRARIES_INIT_ALL, &e);
	P4CHECK(e);
}

void P4Implementation::DisconnectImpl()
{
	if (m_isConnected)
	{
		Error e;
		m_client.Final(&e);
		P4CHECK(e);
	}

	m_isConnected = false;
}

void P4Implementation::ConnectImpl(const std::string& host, const std::string& port, const std::string& user, const std::string& password)
{
	Error e;

	if (!m_isConnected)
	{
		m_client.Final(&e);
		P4CHECK(e);
	}

	m_client.SetHost(host.c_str());
	m_client.SetPort(port.c_str());
	m_client.SetUser(user.c_str());
	m_client.SetPassword(user.c_str());

	m_client.Init(&e);
	P4CHECK(e);
	
	m_isConnected = true;
}

void P4Implementation::AddImpl(const std::filesystem::path& file)
{
	std::string command = std::format("p4 add {}", file.string());
	m_client.Run(command.c_str(), &m_ui);
}

void P4Implementation::DeleteImpl(const std::filesystem::path& file)
{
}

void P4Implementation::SubmitImpl(const std::string& message)
{
}

void P4Implementation::SyncImpl(const std::string& depo)
{
}
