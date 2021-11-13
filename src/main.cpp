#include <iostream>
#include <vector>
#include <string>

#include "Handle.hpp"
#include "injector/Types.hpp"
#include "injector/Injector.hpp"

constexpr i32 opt_datadir_offset = (sizeof(void*) == 8 ? 0x70 : 0x60);

inline bool searchMonoFunction(const HandleView &view, const HModule &module)
{
	return view.searchFunction(module, "mono_get_root_domain") != 0;
}

bool searchMonoModule(const HandleView &view, HModule &monoModule)
{
	std::vector<HModule> hModules = view.enumerateModules(ModulesList::X64);

	for (auto &module : hModules)
	{
		if (searchMonoFunction(view, module))
		{
			std::cout << module.getPath() << std::endl;
			monoModule = module;
			return true;
		}
	}

	return false;
}

extern "C" __declspec(dllexport) MonoAssembly *inject(const char *dllPath, const char *nmspaceName, const char *className, const char *methodName)
{
	if constexpr (sizeof(void*) != 8)
	{
		printf("x32 system doesnt support!\n");
		return nullptr; 
	}

	HANDLE hProcess;
	HModule monoModule;
	HWND hwnd = FindWindowA(nullptr, "Risk of Rain 2");

	DWORD procId = 0; 
	GetWindowThreadProcessId(hwnd, &procId);

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, procId);

	if (!hProcess)
	{
		printf("Not found hProcess\n");
		return nullptr;
	}

	HandleView hv(hProcess);

	if (!searchMonoModule(hv, monoModule)) return nullptr;

	Injector inj(&hv, monoModule);
	MonoAssembly *assm = inj.inject(
		dllPath,
		nmspaceName,
		className,
		methodName	
	);

	return assm;
}

extern "C" __declspec(dllexport) void enject(MonoAssembly *assembly, const char *nmspaceName, const char *className, const char *methodName)
{
	if constexpr (sizeof(void*) != 8)
	{
		printf("x32 system doesnt support!\n");
		return; 
	}

	HANDLE hProcess;
	HModule monoModule;
	HWND hwnd = FindWindowA(nullptr, "Risk of Rain 2");

	DWORD procId = 0; 
	GetWindowThreadProcessId(hwnd, &procId);

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, procId);

	if (!hProcess)
	{
		printf("Not found hProcess\n");
		return;
	}

	HandleView hv(hProcess);

	if (!searchMonoModule(hv, monoModule)) return;

	Injector inj(&hv, monoModule);

	inj.eject(assembly, nmspaceName, className, methodName);
}

int main()
{
	if constexpr (sizeof(void*) != 8)
	{
		MessageBox(nullptr, TEXT("x32 system doesnt support!"), TEXT("OK"), MB_OK);
		return 1; 
	}

	HANDLE hProcess;
	HWND hwnd = FindWindowA(nullptr, "Risk of Rain 2");
	if (!hwnd)
	{
		MessageBox(nullptr, TEXT("Not found hwnd"), TEXT("OK"), MB_OK); 
		return 1;
	}

	DWORD procId = 0; 
	GetWindowThreadProcessId(hwnd, &procId);
	std::cout << procId << std::endl;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, procId);

	if (!hProcess)
	{
		MessageBox(nullptr, TEXT("Not found hProcess"), TEXT("OK"), MB_OK);
		return 1;
	}

	HModule monoModule;
	HandleView hv(hProcess);

	searchMonoModule(hv, monoModule);

	if (!monoModule) return 1;

	Injector inj(&hv, monoModule);
	auto assm = inj.inject(
		"C:\\Users\\endar\\Desktop\\TestInject\\TestInject\\bin\\Debug\\TestInject.dll",
		"TestInject",
		"Loader",
		"Load"	
	);

	Sleep(10000);

	inj.eject(assm, "TestInject", "Loader", "Unload");

	CloseHandle(hProcess);

	MonoAssembly *const a = inject(
		"C:\\Users\\endar\\Desktop\\TestInject\\TestInject\\bin\\Debug\\TestInject.dll",
		"TestInject",
		"Loader",
		"Load"	
	);

	enject(a, "TestInject", "Loader", "Unload");

	return 0;
}
