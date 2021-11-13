#include "Handle.hpp"
#include "VirtualMemory.hpp"

HandleView::HandleView(HANDLE hProcess) : m_hProcess(hProcess), m_allocatedMemory() { }

std::string HandleView::readProcessMemory(const void *address, size_t size) const
{
	std::string a(size, (char)0);
	ReadProcessMemory(m_hProcess, address, a.data(), a.size(), nullptr);
	return a;
}

void HandleView::writeProcessMemory(void *address, const std::string &data)
{
	WriteProcessMemory(m_hProcess, address, data.data(), data.size(), nullptr);
}

std::vector<HMODULE> HandleView::enumerateRawModules(ModulesList filterFlag) const
{
	std::vector<HMODULE> temp;
	DWORD cbNeeded;

	if (!EnumProcessModulesEx(m_hProcess, temp.data(), 0, &cbNeeded, static_cast<DWORD>(filterFlag))) return temp;

	temp.resize(cbNeeded / sizeof(void*));

	if (!EnumProcessModulesEx(m_hProcess, temp.data(), cbNeeded, &cbNeeded, static_cast<DWORD>(filterFlag))) return temp;

	return temp;
}

std::vector<HModule> HandleView::enumerateModules(ModulesList filterFlag) const
{
	std::vector<HModule> res;
	std::vector<HMODULE> temp = enumerateRawModules(filterFlag);

	for (auto &i : temp)
	{
		MODULEINFO info;
		GetModuleInformation(m_hProcess, i, &info, static_cast<DWORD>(sizeof(void*) * temp.size()));
		std::string path = getModuleFileName(i);

		GetModuleFileNameEx(m_hProcess, i, path.data(), static_cast<DWORD>(path.size()));

		res.emplace_back(HModule(i, m_hProcess, info, path));
	}

	return res;
}

uintptr_t HandleView::searchFunction(const HModule &module, const std::string &functionName) const
{
	const char *baseOfDll = static_cast<const char *>(module.getBaseOfDll());

	auto mzHeader = readProcessMemory<IMAGE_DOS_HEADER>(module.getBaseOfDll());
	auto sntHeaders = readProcessMemory<IMAGE_NT_HEADERS>(baseOfDll + mzHeader.e_lfanew);
	auto sexportDir = readProcessMemory<IMAGE_EXPORT_DIRECTORY>(baseOfDll + sntHeaders.OptionalHeader.DataDirectory->VirtualAddress);

	for (i32 i = 0; i < static_cast<i32>(sexportDir.NumberOfNames); i++)
	{
		i32 offset =  readProcessMemory<i32>((baseOfDll + sexportDir.AddressOfNames) + i * 4);
		i16 ordinal = readProcessMemory<i16>((baseOfDll + sexportDir.AddressOfNameOrdinals) + i * 2);
		uintptr_t addr = (uintptr_t)module.getBaseOfDll() + readProcessMemory<i32>((baseOfDll + sexportDir.AddressOfFunctions) + ordinal * 4);
		std::string name = readProcessMemory(baseOfDll + offset, 32);

		if (name.find(functionName) != std::string::npos)
			return addr;
	}

	return 0;
}

std::string HandleView::getModuleFileName(HMODULE module) const
{
	std::string path(MAX_PATH, (char)0);

	GetModuleFileNameEx(m_hProcess, module, path.data(), static_cast<DWORD>(path.size()));

	return path;
}

HANDLE HandleView::getRawHandle() const
{
	return m_hProcess;
}

VirtualMemory HandleView::allocVirtualMemory(size_t size, DWORD allocationType, DWORD protection)
{
	void *mem = VirtualAllocEx(m_hProcess, nullptr, size, allocationType, protection);

	VirtualMemory res(*this, mem, size);
	m_allocatedMemory.emplace_back(res);

	return res;
}

void HandleView::freeAllocatedMemory()
{
	for (auto vm : m_allocatedMemory)
		vm.free();
	
	m_allocatedMemory.clear();
}
