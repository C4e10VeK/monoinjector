#pragma once

#include <Windows.h>
#include "Handle.hpp"

class VirtualMemory
{
private:
    friend class HandleView;
    using VMem = void*;

    const HandleView &m_handle;
    VMem m_address;
    size_t m_size;
public:
    VirtualMemory(const HandleView &handle, VMem address, size_t size) 
        : m_handle(handle), 
          m_address(address),
          m_size(size) { }

    /**
     * @brief Get the Address
     * 
     * @return void*
     */
    [[nodiscard]] VMem getAddress() const noexcept
    {
        return m_address;
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_address != nullptr;
    }

    bool operator!() const noexcept
    {
        return !isValid();
    }

    explicit operator VMem() const
    {
        return m_address;
    }
private:
    /**
     * @brief Free allocated virtual memory
     * 
     * @param freeType 
     */
    void free(DWORD freeType = MEM_DECOMMIT)
    {
        if (m_address == nullptr) return;

        VirtualFreeEx(m_handle.getRawHandle(), m_address, m_size, freeType);
    }
};
