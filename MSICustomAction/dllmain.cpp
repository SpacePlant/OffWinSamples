#pragma comment(lib, "Msi.lib")

#include <Windows.h>
#include <MsiQuery.h>

#include <string>

extern "C" __declspec(dllexport) DWORD __stdcall SignalAndWait(MSIHANDLE handle)
{
	// Get event names from properties
	DWORD buffer_size = 0;
	wchar_t empty = 0;
	if (MsiGetPropertyW(handle, L"CustomActionData", &empty, &buffer_size) != ERROR_MORE_DATA)
	{
		return 0;
	}
	buffer_size++;
	std::wstring buffer(buffer_size - 1, 0);
	if (MsiGetPropertyW(handle, L"CustomActionData", buffer.data(), &buffer_size))
	{
		return 0;
	}
	auto split_pos = buffer.find(L";");
	auto primary_continue_name = buffer.substr(0, split_pos);
	auto msi_continue_name = buffer.substr(split_pos + 1);

	// Signal the primary thread to continue
	HANDLE primary_continue = OpenEventW(EVENT_MODIFY_STATE, false, primary_continue_name.c_str());
	if (!primary_continue)
	{
		return 0;
	}
	SetEvent(primary_continue);
	CloseHandle(primary_continue);

	// Wait for signal to continue
	HANDLE msi_continue = OpenEventW(SYNCHRONIZE, false, msi_continue_name.c_str());
	if (!msi_continue)
	{
		return 0;
	}
	WaitForSingleObject(msi_continue, 10000);
	CloseHandle(msi_continue);

	return 0;
}
