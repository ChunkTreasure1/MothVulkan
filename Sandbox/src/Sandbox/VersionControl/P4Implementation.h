#pragma once

#include "VersionControl.h"

#include <p4/clientapi.h>
#include <p4/p4libs.h>

#define P4CHECK(e) 	if (e.Test()) \
					{ \
						StrBuf msg; \
						e.Fmt(&msg); \
						LP_ERROR("VCS: {0}", msg.Text()); \
						LP_ASSERT(false, ""); \
					} \

class P4Implementation final : public VersionControl
{
public: 
	P4Implementation() = default;

protected:
	void InitializeImpl() override;
	void ShutdownImpl() override;
	void DisconnectImpl() override;
	void ConnectImpl(const std::string& host, const std::string& port, const std::string& user, const std::string& password) override;

	void AddImpl(const std::filesystem::path& file) override;
	void DeleteImpl(const std::filesystem::path& file) override;
	void SubmitImpl(const std::string& message) override;
	void SyncImpl(const std::string& depo /* = "" */) override;

private:
	ClientUser m_ui;
	ClientApi m_client;

	bool m_isConnected = false;
};