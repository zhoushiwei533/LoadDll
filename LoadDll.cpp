// LoadDll.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "LoadDll.h"
#include <windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <direct.h>
#include<set>
#include "psapi.h " 

#pragma comment(lib, "Psapi.lib ")
using namespace std;

#define WECHAT_PROCESS_NAME "WeChat.exe";

INT_PTR CALLBACK Dlgproc(HWND Arg1, UINT Arg2, WPARAM Arg3, LPARAM Arg4);
VOID InjectDll(DWORD pid);
VOID freeDll();
VOID  ProcessNameFindPid(LPCSTR processName);
void GetModuleByPid(DWORD dwPid, set<string> & vData);
set<DWORD> injectPids;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    
	DialogBox(hInstance,MAKEINTRESOURCE(ID_MAIN),NULL, &Dlgproc);
    return 0;
}


INT_PTR CALLBACK Dlgproc(HWND handle,UINT uMsg,WPARAM wParam,LPARAM Arg4)
{
	if (uMsg == WM_INITDIALOG) {
		//MessageBox(NULL,"首次加载","标题",0);
	
	}
	if (uMsg==WM_CLOSE) {
		EndDialog(handle,0);
	}
	//按钮操作
	if (uMsg == WM_COMMAND) {
		if (wParam == ID_INJECT) {
			//InjectDll();
			ProcessNameFindPid("WeChat.exe");
		}

		if (wParam == ID_UNBIND) {
			freeDll();
		}

	}
	return FALSE;
}

//获取微信句柄 
//进程名称获取pid
VOID  ProcessNameFindPid(LPCSTR processName) {
	//获取系统进程快照，然后查找进程名
	HANDLE  processList = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,NULL);
	PROCESSENTRY32 processInfo = {0};
	processInfo.dwSize = sizeof(processInfo);
	do
	{
		if(strcmp(processName, processInfo.szExeFile) == 0) {
			InjectDll(processInfo.th32ProcessID);
			//return  processInfo.th32ProcessID;
		}
	} while (Process32Next(processList, &processInfo));
	//return 0;
}
//申请内存放dll的路径 然后通过pid打开微信进程获取到进程句柄

VOID InjectDll(DWORD pid) {
	//获取微信句柄
	//DWORD pid = ProcessNameFindPid("WeChat.exe");
	
	if (pid == 0) {
		MessageBox(NULL, "微信没有启动", "警告", 0);
		return;
	}

	char *buffer = NULL;
	char path[0x100] = { 0 };
	if ((buffer = _getcwd(NULL, 0)) == NULL) {
		perror("获取路径错误");
	}
	else {
		sprintf_s(path, "%s\\WechatTools.dll", buffer);
	}

	set<string> dllPaths;
	GetModuleByPid(pid, dllPaths);
	if (dllPaths.find(path) != dllPaths.end()) {
		MessageBox(NULL, "已经注入，不需要重复注入", "警告", 0);
		return;
	}

	// 
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS,FALSE,pid);
	if (NULL == process) {
		MessageBox(NULL, "进程打开失败", "警告", 0);
		return;
	}


	
	//申请内存
	LPVOID dllAdd = VirtualAllocEx(process,NULL, strlen(path),MEM_COMMIT,PAGE_READWRITE);
	if (NULL == dllAdd) {
		MessageBox(NULL, "内存地址申请失败", "警告", 0);
		return;
	}
	CHAR test[0x100] = {0};
	sprintf_s(test, "写入的地址是:%p", dllAdd);
	OutputDebugString(test);
	//写入dll
	if (WriteProcessMemory(process, dllAdd, path, strlen(path), NULL) == 0) {
		MessageBox(NULL, "dll写入失败", "警告", 0);
		return;
	}
	HMODULE k32 = GetModuleHandle("Kernel32.dll");
	FARPROC loadAdd = GetProcAddress(k32,"LoadLibraryA");
	
	MessageBox(NULL, "注入成功", "警告", 0);
	CreateRemoteThread(process,NULL,0,(LPTHREAD_START_ROUTINE)loadAdd, dllAdd,0,NULL);

	injectPids.insert(pid);

}


void GetModuleByPid(DWORD dwPid, set<string> & vData)
{
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
	if (hProc)
	{
		HMODULE hMod[1024];
		DWORD cbNeeded;
		CHAR szModName[MAX_PATH] = { 0 };
		CHAR lPath[MAX_PATH] = { 0 };
		// 获取进程模块
		if (EnumProcessModules(hProc, hMod, sizeof(hMod), &cbNeeded))
		{
			//获得模块路径
			for (int i = 0; i <= cbNeeded / sizeof(HMODULE); ++i)
			{

				if (GetModuleFileNameExA(hProc, hMod[i], szModName, sizeof(szModName)))
				{
					vData.insert(szModName);

				}
			}


		}


		CloseHandle(hProc);

	}
}


//卸载FreeLibrary

VOID freeDll() {
	//HMODULE k32 = GetModuleHandle("Kernel32.dll");
	//FreeLibrary(k32);
}



//写入dll 通过远程线程执行函数 执行loadLiaray执行dll




