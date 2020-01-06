#include "AppMain.h"
#include "glog/logging.h"
#include "base/PathService.h"
#include "base/system_info.h"
#include "base/asynchronous_task_dispatch.h"

#include "DataReport/DataReport.h"

Application::Application(HINSTANCE app_instance)
{
	//Initialize Com API
	::CoInitialize(0);
	//Initialize ActiveX
	::OleInitialize(nullptr);

	//Initialize google log
	init_glog();
	
	//Async Tash Dispatcher
	AsyncTaskDispatch::init_main_thread_dispatcher();

	//DUI Instance Set
	//CPaintManagerUI::SetInstance(app_instance);
}

Application::~Application()
{
	//destroy process mutex
	if (singleton_process_runnging_lock_)
	{
		CloseHandle(singleton_process_runnging_lock_);
		singleton_process_runnging_lock_ = nullptr;
	}

	//destroy process share memory
	if (singleton_process_exiting_share_memory_)
	{
		CloseHandle(singleton_process_exiting_share_memory_);
		singleton_process_exiting_share_memory_ = nullptr;
	}

	::CoUninitialize();
	::OleUninitialize();
}

//Application Main Loop
int Application::run()
{
	//Singleton
	if (!handle_singleton_process())
		return 1;
	
	DataReport::global()->report_event(APP_STARTUP_EVENT);

	//MC Protocol
	if (!RegisterMCProtocol())
	{
		DataReport::global()->report_event(REGISTER_MC_PROTOCOL_FAILED_EVENT);
	}

	//Browser Setting
	//TODO

	//AppUpdate
	//TODO

	//AppConfig
	//TODO

	//IPv6 ygg
	//TODO

	//Network Dispatcher
	//TODO

	//Self Resource Extraction
	//lambda
	std::thread extrace_resource([]()
		{

		});
	extrace_resource.detach();

	//Main Wnd
	//TODO

	//Exit Process

	signal_exiting();

	DataReport::global()->report_event(APP_EXIT_EVENT);



	return 0;
}

//Google Log
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

//Singleton process
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

	//Create Mutex 
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

bool Application::RegisterMCProtocol()
{
	bool ret_val = true;
	CRegKey key;
	auto ret = key.Create(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\mc\\shell\\open\\command");
	if (ret == 0)
	{
		wchar_t path[1024] = { 0 };
		GetModuleFileNameW(nullptr, path, 1024);
		std::wstring val = L"\"";
		val += path;
		val += L"\" %1";
		key.SetStringValue(NULL, val.c_str(), REG_SZ);

		key.Close();
	}
	else
	{
		ret_val = false;
		LOG(ERROR) << "create reg key formc protocol failed with err " << ret;
	}

	ret = key.Open(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\mc", KEY_WRITE | KEY_WOW64_64KEY);
	if (0 == ret)
	{
		auto ret1 = key.SetStringValue(NULL, L"URL:mc Protocol", REG_SZ);
		auto ret2 = key.SetStringValue(L"URL Protocol", L"", REG_SZ);
		if (ret1 != 0 || ret2 != 0)
		{
			ret_val = false;
			LOG(ERROR) << "set reg key value for mc protocol failed with err " << ret1 << " " << ret2;
		}
	}
	else
	{
		ret_val = false;
		LOG(ERROR) << "open reg key for mc protocol failed with err " << ret;
	}

	return ret_val;
}

//Win Main
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine, int nCmdShow)
{
	Application app(hInstance);

	return app.run();
}