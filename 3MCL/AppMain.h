#pragma once
#include "curl_global.h"
#include "base/sigslot.h"

//struct IUnknown; //be sure ConformanceMode set false in vcxproject
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <shlwapi.h>
#include <codecvt>
#include <atlbase.h>
#include <Strsafe.h>

class Application
	: public sigslot::has_slots<>
{
public:
	Application(HINSTANCE app_instance);
	~Application();

public:
	int run();

protected:
	void init_glog();

	bool RegisterMCProtocol();

	//void EnableIEFeatures();

	//void EnableMiniblink();

	//bool EnablePythonEmbed();

	//void OnDownloadUpdateSuccess(std::wstring update_exe_path, std::wstring app_name, std::wstring version, std::wstring note);
	
	bool handle_singleton_process();

	void signal_exiting();

private:

	HANDLE singleton_process_runnging_lock_ = nullptr;
	HANDLE singleton_process_exiting_share_memory_ = nullptr;

	curl::curl_global curl_global_;
};
