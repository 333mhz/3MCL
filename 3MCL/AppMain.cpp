#include "AppMain.h"
#include "glog/logging.h"
#include "base/PathService.h"

Application::Application(HINSTANCE app_instance)
{
	::OleInitialize(nullptr);
	::CoInitialize(0);

	init_glog();
	//AsyncTaskDispatch::init_main_thread_dispatcher();
	//CPaintManagerUI::SetInstance(app_instance);
}

Application::~Application()
{
	if (singleton_process_runnging_lock_)
	{
		CloseHandle(singleton_process_runnging_lock_);
		singleton_process_runnging_lock_ = nullptr;
	}

	if (singleton_process_exiting_share_memory_)
	{
		CloseHandle(singleton_process_exiting_share_memory_);
		singleton_process_exiting_share_memory_ = nullptr;
	}

	::CoUninitialize();
	::OleUninitialize();
}

int Application::run()
{
	return 0;
}

void Application::init_glog()
{
	google::InitGoogleLogging("3MCL");

#ifdef _DEBUG
	google::SetStderrLogging(google::GLOG_WARNING);
#endif

	std::wstring log_dir = PathService().exe_dir();
	log_dir.append(L"\\log");
	::SHCreateDirectory(0, log_dir.c_str());
	char temp[1024] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, log_dir.c_str(), -1, temp, 1024, NULL, NULL);
	static std::string log_dir_str = temp;
	FLAGS_log_dir = log_dir_str;

	google::SetLogDestination(google::GLOG_WARNING, "");
	google::SetLogDestination(google::GLOG_ERROR, "");
	google::SetLogDestination(google::GLOG_FATAL, "");
}

bool Application::handle_singleton_process()
{
	HANDLE exist_exiting_process_handle = ::OpenFileMappingW(FILE_MAP_READ, FALSE, L"333mhz_minecraft_launcher_exiting_map");
	if (exist_exiting_process_handle)
	{
		LPVOID ptr = ::MapViewOfFile(exist_exiting_process_handle, FILE_MAP_READ, 0, 0, 0);
		if (ptr)
		{
			DWORD pid = *(DWORD*)ptr;
			HANDLE process = ::OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
			if (process)
			{
				::TerminateProcess(process, 1);
				::WaitForSingleObject(process, 5 * 1000);
				::CloseHandle(process);
			}
			::UnmapViewOfFile(ptr);
		}
		::CloseHandle(exist_exiting_process_handle);
		exist_exiting_process_handle = nullptr;
	}

	singleton_process_runnging_lock_ = ::CreateMutexW(nullptr, TRUE, L"333mhz_minecraft_launcher_running");
	auto err = ::GetLastError();
	if (err == ERROR_ALREADY_EXISTS || err == ERROR_ACCESS_DENIED)
	{
		HWND main_win = ::FindWindowW(L"333mhz_minecraft_launcher_mainwnd", L"3MCL"/*mcfe::g_app_name*/);
		if (main_win)
		{
			::ShowWindow(main_win, SW_RESTORE);
			::ShowWindow(main_win, SW_SHOW);
			::SetForegroundWindow(main_win);
			::InvalidateRect(main_win, nullptr, FALSE);
			COPYDATASTRUCT cp;
			//cp.dwData = MSG_PASS_APP_PARAM;
			auto cmd = GetCommandLineW();
			cp.lpData = cmd;
			cp.cbData = (wcslen(cmd) + 1) * sizeof(wchar_t);
			::SendMessageW(main_win, WM_COPYDATA, 0, (LPARAM)&cp);
		}
		return false;
	}
	else if (singleton_process_runnging_lock_ == nullptr)
	{
		::MessageBox(0, 0, 0, 0);
	}

	return true;
}

void Application::signal_exiting()
{
	singleton_process_exiting_share_memory_ = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(DWORD), L"333mhz_minecraft_launcher_exiting_map");
	LPVOID ptr = ::MapViewOfFile(singleton_process_exiting_share_memory_, FILE_MAP_WRITE, 0, 0, 0);
	if (ptr)
	{
		*(DWORD*)ptr = ::GetCurrentProcessId();
		::UnmapViewOfFile(ptr);
	}
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine, int nCmdShow)
{
	Application app(hInstance);
	return app.run();
}