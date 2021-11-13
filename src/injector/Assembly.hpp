#pragma once

#include <vector>
#include "Types.hpp"

class Assembly
{
private:
    std::vector<ui8> m_asm;
public:
    Assembly() : m_asm() {}
    
    [[nodiscard]] const std::vector<ui8> &getAssembly() const { return m_asm; }

    void MovToRax(std::vector<ui8> &data)
    {
        m_asm.emplace_back(0x48);
        m_asm.emplace_back(0xb8);

        insertRange(data);
    }

    void MovRaxTo(std::vector<ui8> &data)
    {
        m_asm.emplace_back(0x48);
        m_asm.emplace_back(0xa3);

        insertRange(data);
    }

    void MovToRcx(std::vector<ui8> &data)
    {
        m_asm.emplace_back(0x48);
        m_asm.emplace_back(0xb9);

        insertRange(data);
    }

    void MovToRdx(std::vector<ui8> &data)
    {
        m_asm.emplace_back(0x48);
        m_asm.emplace_back(0xba);

        insertRange(data);
    }

    void MovToR8(std::vector<ui8> &data)
    {
        m_asm.emplace_back(0x49);
        m_asm.emplace_back(0xb8);

        insertRange(data);
    }

    void MovToR9(std::vector<ui8> &data)
    {
        m_asm.emplace_back(0x49);
        m_asm.emplace_back(0xb9);

        insertRange(data);
    }

    void callRax()
    {
        m_asm.emplace_back(0xff);
        m_asm.emplace_back(0xd0);
    }

    void subRsp(ui8 data)
    {
        m_asm.emplace_back(0x48);
        m_asm.emplace_back(0x83);
        m_asm.emplace_back(0xec);
        m_asm.emplace_back(data);
    }

    void addRsp(ui8 data)
    {
        m_asm.emplace_back(0x48);
        m_asm.emplace_back(0x83);
        m_asm.emplace_back(0xc4);
        m_asm.emplace_back(data);
    }

    void retrn()
    {
        m_asm.emplace_back(0xc3);
    }
private:
    void resizeAsmArray(const std::vector<ui8> &data)
    {
        m_asm.reserve(m_asm.size() + data.size());
    }

    void insertRange(std::vector<ui8> &data)
    {
        resizeAsmArray(data);
        m_asm.insert(m_asm.end(), data.begin(), data.end());
    }
};
