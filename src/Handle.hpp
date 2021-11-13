#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#include "HModule.hpp"
#include "injector/Types.hpp"

enum class ModulesList : DWORD
{
	DEFAULT = LIST_MODULES_DEFAULT,
	ALL = LIST_MODULES_ALL,
	X64 = LIST_MODULES_64BIT,
	X32 = LIST_MODULES_32BIT
};

class VirtualMemory;

class HandleView
{
private:
	HANDLE m_hProcess{};
	std::vector<VirtualMemory> m_allocatedMemory;
public:
	HandleView() = default;
	explicit HandleView(HANDLE hProcess);
	virtual ~HandleView() = default;

	/**
	 * @brief Get the WinAPI HANDLE
	 * 
	 * @return HANDLE 
	 */
	HANDLE getRawHandle() const;

	/**
	 * @brief Read memory from process by address
	 * 
	 * @tparam T Type to read from memory
	 * @param address Address reading memory
	 * @return T 
	 */
	template<typename T>
	T readProcessMemory(const void *address) const
	{
		T res;
		ReadProcessMemory(m_hProcess, address, &res, sizeof(T), nullptr);

		return res;
	}

	template<typename T>
	std::vector<T> readProcessMemory(const void *address, size_t length) const
	{
		std::vector<T> res(length);
		ReadProcessMemory(m_hProcess, address, &res, length * sizeof(T), nullptr);

		return res;
	}

	/**
	 * @brief Read memory from process by address
	 * 
	 * @param address Address reading memory
	 * @param size String size
	 * @return std::string 
	 */
	std::string readProcessMemory(const void *address, size_t size) const;

	template<typename T>
	void writeProcessMemory(void *address, const T data)
	{
		WriteProcessMemory(m_hProcess, address, &data, sizeof(T), nullptr);
	}

	void writeProcessMemory(void *address, const std::string &data);

	template<typename T>
	void writeProcessMemory(void *address, const std::vector<T> &data)
	{
		WriteProcessMemory(m_hProcess, address, data.data(), data.size() * sizeof(T), nullptr);
	}

	template<typename T, size_t N>
	void writeProcessMemory(void *address, const std::array<T, N> &data)
	{
		WriteProcessMemory(m_hProcess, address, data.data(), data.size() * sizeof(T), nullptr);
	}

	/**
	 * @brief Enumerate modules as WinApi HMODULE
	 * 
	 * @param filterFlag Filter modules
	 * @return std::vector<HMODULE> 
	 */
	std::vector<HMODULE> enumerateRawModules(ModulesList filterFlag) const;

	/**
	 * @brief Enumerate modules as custom class HModule
	 * 
	 * @param filterFlag Filter modules
	 * @return std::vector<HModule> 
	 */
	std::vector<HModule> enumerateModules(ModulesList filterFlag) const;

	uintptr_t searchFunction(const HModule &module, const std::string &functionName) const;

	VirtualMemory allocVirtualMemory(size_t size, DWORD allocationType = MEM_COMMIT, DWORD protection = PAGE_EXECUTE_READWRITE);

	void freeAllocatedMemory();

private:
	std::string getModuleFileName(HMODULE module) const;
};
