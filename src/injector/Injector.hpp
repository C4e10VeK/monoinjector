#pragma once

#include <Windows.h>
#include <cstdint>
#include <array>
#include <utility>

#include "../Handle.hpp"
#include "../VirtualMemory.hpp"

#include "Assembly.hpp"

using MonoObject 		  = void;
using MonoDomain 		  = void;
using MonoAssembly 		  = void;
using MonoImage 		  = void;
using MonoClass 		  = void;
using MonoMethod 		  = void;

class Injector
{
private:
	HandleView *m_handle{};
	HModule m_module;
	MonoDomain *m_rootDomain{};

	enum class MonoFunction : size_t
	{
		THREAD_ATTACH = 0,
		GET_ROOT_DOMAIN,
		ASSEMBLY_GET_IMAGE,
		CLASS_FROM_NAME,
		CLASS_GET_METHOD_FROM_NAME,
		RUNTIME_INVOKE,
		IMAGE_OPEN,
		ASSEMBLY_LOAD_FROM
	};

	std::array<uintptr_t, 8> m_monoFunctions{};
	bool m_attach{};

	enum class MonoImageOpenStatus
	{
		MONO_IMAGE_OK,
		MONO_IMAGE_ERROR_ERRNO,
		MONO_IMAGE_MISSING_ASSEMBLYREF,
		MONO_IMAGE_IMAGE_INVALID
	};

public:
	Injector() = default;
	Injector(HandleView *handle, HModule monoModule) 
			: m_handle(handle), 
			  m_module(std::move(monoModule)),
			  m_rootDomain(nullptr), 
			  m_monoFunctions(), 
			  m_attach(false) { }

	MonoDomain *getMonoDomain()
	{
		return (MonoDomain*)call(MonoFunction::GET_ROOT_DOMAIN, std::vector<uintptr_t>());
	}

	MonoImage *imageOpen(const std::string &path)
	{
		MonoImage *res;
		VirtualMemory pathMem = m_handle->allocVirtualMemory(path.size(), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		VirtualMemory statusMem = m_handle->allocVirtualMemory(sizeof(i32), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		m_handle->writeProcessMemory(static_cast<void *>(pathMem), path);

		res = (MonoImage*)call(MonoFunction::IMAGE_OPEN, std::vector<uintptr_t>{(uintptr_t)pathMem.getAddress(), (uintptr_t)statusMem.getAddress()});

		auto status = static_cast<MonoImageOpenStatus>(m_handle->readProcessMemory<i32>(static_cast<void *>(statusMem)));

		if (status != MonoImageOpenStatus::MONO_IMAGE_OK) return nullptr;

		return res;
	}

	MonoClass *getClassFromName(MonoImage *image, const std::string &nspace, const std::string &className)
	{
		VirtualMemory nspaceMem = m_handle->allocVirtualMemory(nspace.size());
		VirtualMemory classNameMem = m_handle->allocVirtualMemory(className.size());

		m_handle->writeProcessMemory(static_cast<void *>(nspaceMem), nspace);
		m_handle->writeProcessMemory(static_cast<void *>(classNameMem), className);

		return (MonoClass*)call(MonoFunction::CLASS_FROM_NAME, std::vector<uintptr_t>{
			(uintptr_t)image, (uintptr_t)nspaceMem.getAddress(), (uintptr_t)classNameMem.getAddress()});
	}

	MonoMethod *getMethodFromName(MonoClass *c, const std::string &methodName)
	{
		VirtualMemory methodNameMem = m_handle->allocVirtualMemory(methodName.size());
		m_handle->writeProcessMemory(static_cast<void *>(methodNameMem), methodName);

		return (MonoMethod*)call(
			MonoFunction::CLASS_GET_METHOD_FROM_NAME, 
			std::vector<uintptr_t>{
				(uintptr_t)c, (uintptr_t)methodNameMem.getAddress(), 0
			}
		);
	}

	MonoObject *runtimeInvoke(MonoMethod *method)
	{
		return (MonoObject*)call(MonoFunction::RUNTIME_INVOKE, std::vector<uintptr_t>{(uintptr_t)method, 0, 0, 0});
	}

	MonoAssembly *openAssemblyFromImage(MonoImage *image)
	{
		MonoAssembly *res;
		VirtualMemory statusMem = m_handle->allocVirtualMemory(sizeof(i32), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		VirtualMemory m = m_handle->allocVirtualMemory(sizeof(i8), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		m_handle->writeProcessMemory<i8>(static_cast<void *>(m), 0);

		res = (MonoAssembly*)call(MonoFunction::ASSEMBLY_LOAD_FROM, 
								  std::vector<uintptr_t>{
									  (uintptr_t)image, 
								  	  (uintptr_t)m.getAddress(), 
								  	  (uintptr_t)statusMem.getAddress()
								  });
		
		auto status = static_cast<MonoImageOpenStatus>(m_handle->readProcessMemory<i32>(static_cast<void *>(statusMem)));

		if (status != MonoImageOpenStatus::MONO_IMAGE_OK) return nullptr;

		return res;
	}

	MonoImage *getImage(MonoAssembly *assembly)
	{
		return (MonoImage*)call(MonoFunction::ASSEMBLY_GET_IMAGE, std::vector<uintptr_t>{(uintptr_t)assembly});
	}

	MonoAssembly *inject(const std::string &dllPath, const std::string &nspaceName, const std::string &className, const std::string &methodName)
	{
		enumerateMonoFunctions();

		m_rootDomain = getMonoDomain();
		auto image = imageOpen(dllPath);
		m_attach = true;
		auto assembly = openAssemblyFromImage(image);
		auto monClass = getClassFromName(image, nspaceName, className);
		if (monClass == nullptr) return nullptr;
		auto method = getMethodFromName(monClass, methodName);
		if (method == nullptr) return nullptr;
		runtimeInvoke(method);
		
		m_handle->freeAllocatedMemory();

		return assembly;
	}

	void eject(MonoAssembly *assembly, const std::string &nspaceName, const std::string &className, const std::string &methodName)
	{
		if (assembly == nullptr) return;

		enumerateMonoFunctions();

		m_rootDomain = getMonoDomain();
		m_attach = true;
		auto image = getImage(assembly);
		if (image == nullptr) return;
		auto monClass = getClassFromName(image, nspaceName, className);
		if (monClass == nullptr) return;
		auto method = getMethodFromName(monClass, methodName);
		if (method == nullptr) return;
		runtimeInvoke(method);
		
		m_handle->freeAllocatedMemory();
	}

private:
	void enumerateMonoFunctions()
	{
		m_monoFunctions[static_cast<size_t>(MonoFunction::THREAD_ATTACH)] = m_handle->searchFunction(m_module, "mono_thread_attach");
		m_monoFunctions[static_cast<size_t>(MonoFunction::GET_ROOT_DOMAIN)] = m_handle->searchFunction(m_module, "mono_get_root_domain");
		m_monoFunctions[static_cast<size_t>(MonoFunction::ASSEMBLY_GET_IMAGE)] = m_handle->searchFunction(m_module, "mono_assembly_get_image");
		m_monoFunctions[static_cast<size_t>(MonoFunction::CLASS_FROM_NAME)] = m_handle->searchFunction(m_module, "mono_class_from_name");
		m_monoFunctions[static_cast<size_t>(MonoFunction::CLASS_GET_METHOD_FROM_NAME)] = m_handle->searchFunction(m_module, "mono_class_get_method_from_name");
		m_monoFunctions[static_cast<size_t>(MonoFunction::RUNTIME_INVOKE)] = m_handle->searchFunction(m_module, "mono_runtime_invoke");
		m_monoFunctions[static_cast<size_t>(MonoFunction::IMAGE_OPEN)] = m_handle->searchFunction(m_module, "mono_image_open");
		m_monoFunctions[static_cast<size_t>(MonoFunction::ASSEMBLY_LOAD_FROM)] = m_handle->searchFunction(m_module, "mono_assembly_load_from");
	}

	template<typename T = uintptr_t>
	uintptr_t call(MonoFunction func, std::vector<T> args)
	{
		if (m_monoFunctions[static_cast<size_t>(func)] == 0) return 0;
		
		Assembly asmCode;
		VirtualMemory resMem = m_handle->allocVirtualMemory(sizeof(uintptr_t));

		if (!resMem) return 0;

		m_handle->writeProcessMemory<uintptr_t>(resMem.getAddress(), 0);

		asmCode.subRsp(40);

		if (m_attach && m_rootDomain != nullptr)
		{
			auto bytesTAttach = getBytes(m_monoFunctions[static_cast<size_t>(MonoFunction::THREAD_ATTACH)]);
			asmCode.MovToRax(bytesTAttach);
			auto bytesRootDomain = getBytes((uintptr_t)m_rootDomain);
			asmCode.MovToRcx(bytesRootDomain);
			asmCode.callRax();
		}

		auto bytesPtr = getBytes(m_monoFunctions[static_cast<size_t>(func)]);
		asmCode.MovToRax(bytesPtr);

		for (size_t i = 0; i < args.size(); i++)
		{
			switch (i)
			{
			case 0:
				{
					auto bytesArgs = getBytes(args[i]);
					asmCode.MovToRcx(bytesArgs);
				}
				break;
			case 1:
				{
					auto bytesArgs = getBytes(args[i]);
					asmCode.MovToRdx(bytesArgs);
				}
				break;
			case 2:
				{
					auto bytesArgs = getBytes(args[i]);
					asmCode.MovToR8(bytesArgs);
				}
				break;
			case 3:
				{
					auto bytesArgs = getBytes(args[i]);
					asmCode.MovToR9(bytesArgs);
				}
				break;
			default:
				break;
			}
		}
		
		asmCode.callRax();

		asmCode.addRsp(40);
		
		bytesPtr = getBytes((uintptr_t)resMem.getAddress());
		asmCode.MovRaxTo(bytesPtr);

		asmCode.retrn();

		VirtualMemory asmCodeMem = m_handle->allocVirtualMemory(asmCode.getAssembly().size());

		if (!asmCodeMem) return 0;
		m_handle->writeProcessMemory(static_cast<void *>(asmCodeMem), asmCode.getAssembly());

		HANDLE rt = CreateRemoteThread(m_handle->getRawHandle(), nullptr, 0, (LPTHREAD_START_ROUTINE)asmCodeMem.getAddress(), nullptr, 0, nullptr);
		
		if (WaitForSingleObject(rt, INFINITE) != WAIT_OBJECT_0)
		{
			CloseHandle(rt);
			return 0;
		}

		auto res = m_handle->readProcessMemory<uintptr_t>(static_cast<void *>(resMem));

		CloseHandle(rt);

		return res;
	}

	template<typename T>
	static std::vector<ui8> getBytes(T value)
	{
		std::vector<ui8> res(sizeof(T));
		*(T *)res.data() = value;
		return res;
	} 
};