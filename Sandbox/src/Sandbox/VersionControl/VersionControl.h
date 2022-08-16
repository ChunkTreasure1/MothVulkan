#pragma once

#include <Lamp/Core/Base.h>

struct VersionControlSettings
{
	std::string host;
	std::string port;
	std::string user;
	std::string password;
};

enum class VersionControlSystem
{
	Perforce
};

class VersionControl
{
public:
	static void Initialize(VersionControlSystem system);
	static void Shutdown();

	static void Connect(const std::string& host, const std::string& port, const std::string& user, const std::string& password);
	static void Disconnect();

	static void Add(const std::filesystem::path& file);
	static void Delete(const std::filesystem::path& file);
	static void Submit(const std::string& message);
	static void Sync(const std::string& depo = "");

protected:
	virtual void InitializeImpl() = 0;
	virtual void ShutdownImpl() = 0;
	virtual void DisconnectImpl() = 0;
	virtual void ConnectImpl(const std::string& host, const std::string& port, const std::string& user, const std::string& password) = 0;

	virtual void AddImpl(const std::filesystem::path& file) = 0;
	virtual void DeleteImpl(const std::filesystem::path& file) = 0;
	virtual void SubmitImpl(const std::string& message) = 0;
	virtual void SyncImpl(const std::string& depo = "") = 0;

private:
	inline static Scope<VersionControl> s_implementation;
};