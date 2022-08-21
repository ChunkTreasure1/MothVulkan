#pragma once

#include <windows.h>
#include <commdlg.h>
#include <filesystem>

#include <shellapi.h>
#include <shlobj.h>
#include <windows.h>

class FileSystem
{
public:
	static std::filesystem::path GetAssetsPath()
	{
		return "Assets";
	}

	static std::filesystem::path GetEnginePath()
	{
		return "Engine";
	}

	static std::filesystem::path GetShadersPath()
	{
		return "Engine/Shaders";
	}

	static std::filesystem::path GetRenderPipelinesPath()
	{
		return "Engine/RenderPipelines";
	}

	static std::filesystem::path GetRenderPassesPath()
	{
		return "Engine/RenderPasses";
	}

	static std::filesystem::path GetPathRelativeToBaseFolder(const std::filesystem::path& aPath)
	{
		return std::filesystem::relative(aPath, std::filesystem::current_path());
	}

	static bool IsWriteable(const std::filesystem::path& aPath)
	{
		std::filesystem::file_status status = std::filesystem::status(aPath);
		return (status.permissions() & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
	}

	static bool Copy(const std::filesystem::path& aSource, const std::filesystem::path& aDestination)
	{
		if (!Exists(aSource))
		{
			return false;
		}

		std::filesystem::copy(aSource, aDestination);
		return true;
	}

	static bool CopyFile(const std::filesystem::path& aSource, const std::filesystem::path& aDestDir)
	{
		if (!Exists(aDestDir))
		{
			return false;
		}

		std::filesystem::path newPath = aDestDir / aSource.filename();

		if (!Exists(newPath))
		{
			Copy(aSource, newPath);
			return true;
		}

		return false;
	}

	static bool Exists(const std::filesystem::path& aPath)
	{
		return std::filesystem::exists(aPath);
	}

	static bool Rename(const std::filesystem::path& aPath, const std::string& aName)
	{
		if (!Exists(aPath))
		{
			return false;
		}

		const std::filesystem::path newPath = aPath.parent_path() / (aName + aPath.extension().string());
		std::filesystem::rename(aPath, newPath);

		return true;
	}

	static bool Move(const std::filesystem::path& file, const std::filesystem::path& destinationFolder)
	{
		if (!Exists(file))
		{
			return false;
		}

		const std::filesystem::path newPath = destinationFolder / file.filename();
		std::filesystem::rename(file, newPath);

		return true;
	}

	static bool CreateFolder(const std::filesystem::path& folder)
	{
		if (Exists(folder))
		{
			return false;
		}

		std::filesystem::create_directories(folder);
	
		return true;
	}

	static bool ShowDirectoryInExplorer(const std::filesystem::path& aPath)
	{
		auto absolutePath = std::filesystem::canonical(aPath);
		if (!Exists(absolutePath))
		{
			return false;
		}

		ShellExecute(NULL, L"explore", absolutePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		return true;
	}

	static bool ShowFileInExplorer(const std::filesystem::path& aPath)
	{
		auto absolutePath = std::filesystem::canonical(aPath);
		if (!Exists(absolutePath))
		{
			return false;
		}

		std::string cmd = "explorer.exe /select,\"" + absolutePath.string() + "\"";
		system(cmd.c_str());

		return true;
	}

	static bool OpenFileExternally(const std::filesystem::path& aPath)
	{
		auto absolutePath = std::filesystem::canonical(aPath);
		if (!Exists(absolutePath))
		{
			return false;
		}

		ShellExecute(NULL, L"open", absolutePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		return true;
	}

	static std::filesystem::path OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = GetActiveWindow();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}

		return std::string();
	}

	static std::filesystem::path OpenFolder()
	{
		TCHAR path[MAX_PATH];

		BROWSEINFO bi = { 0 };
		bi.lpszTitle = (L"Open folder...");
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

		LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

		if (pidl != 0)
		{
			//get the name of the folder and put it in path
			SHGetPathFromIDList(pidl, path);

			//free memory used
			IMalloc* imalloc = 0;
			if (SUCCEEDED(SHGetMalloc(&imalloc)))
			{
				imalloc->Free(pidl);
				imalloc->Release();
			}

			return std::filesystem::path(path);
		}

		return "";
	}

	static std::filesystem::path SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = GetActiveWindow();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}

		return std::string();
	}
};