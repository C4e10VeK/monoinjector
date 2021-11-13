#pragma once

#include <Windows.h>
#include <Psapi.h>
#include <string>
#include <utility>

class HModule
{
private:
	HMODULE m_hModule{};
	[[maybe_unused]] HANDLE lm_hProcess{};
	MODULEINFO m_info{};
	std::string m_path;
public:
	HModule() = default;
	HModule(HMODULE module, HANDLE handle, MODULEINFO info, std::string path)
		: m_hModule(module), 
		  lm_hProcess(handle), 
		  m_info(info),
		  m_path(std::move(path)) {}

	/**
	 * @brief Get the Base Of Dll address
	 * 
	 * @return void*
	 */
	[[nodiscard]] void *getBaseOfDll() const { return m_info.lpBaseOfDll; }
	
	/**
	 * @brief Get the Entry Point address
	 * 
	 * @return void* 
	 */
	[[nodiscard]] void *getEntryPoint() const { return m_info.EntryPoint; }
	
	/**
	 * @brief Get the Size Img 
	 * 
	 * @return DWORD 
	 */
	[[nodiscard]] DWORD getSizeImg() const { return m_info.SizeOfImage; }

	/**
	 * @brief Get the Path
	 * 
	 * @return const std::string& 
	 */
	[[nodiscard]] const std::string &getPath() const { return m_path; }

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_hModule != nullptr;
    }
    
    bool operator!() const noexcept
    {
        return !isValid();
    }
};