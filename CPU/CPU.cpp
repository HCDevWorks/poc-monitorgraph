#include "pch.h"

#include "cpu.h"
//#include "logger.h"

#include <windows.h>
#include <pdh.h>
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wbemuuid.lib")

#include <nlohmann/json.hpp>

// TODO: Serializar os dados para um JSON utilizando a lib nlohmann/json ( Feito )
// Usar o Logger aqui nas funções para registrar informações

namespace CPU {
    // Função para coletar uso geral da CPU
    MONITORGRAPH_API double GetCPUUsage() {
        PDH_HQUERY query;
        PDH_HCOUNTER counter;
        PDH_FMT_COUNTERVALUE counterValue;

        // Cria uma consulta PDH
        if (PdhOpenQuery(NULL, 0, &query) != ERROR_SUCCESS) {
            std::cerr << "Falha ao criar consulta PDH.\n";
            return -1;
		}

        // Adiciona o contador para uso total da CPU
        if (PdhAddCounter(query, L"\\Processor(_Total)\\% Processor Time", 0, &counter) != ERROR_SUCCESS) {
            std::cerr << "Falha ao adicionar contador PDH.\n";
            PdhCloseQuery(query);
            return -1;
        }

        // Coleta dados pela primeira vez (necessário para inicializar o contador)
        PdhCollectQueryData(query);
        Sleep(1000); // Aguarda 1 segundo para obter um valor válido
        PdhCollectQueryData(query);

        // Obtém o valor formatado do contador
        if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &counterValue) != ERROR_SUCCESS) {
            std::cerr << "Falha ao formatar valor do contador.\n";
            PdhCloseQuery(query);
            return -1;
        }

        // Fecha a consulta
        PdhCloseQuery(query);

        return counterValue.doubleValue;
    }

    // Função para obter informações sobre núcleos e threads
    MONITORGRAPH_API nlohmann::json GetCPUInfo() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        // Inicializa a estrutura JSON
        nlohmann::json cpuInfo;
        cpuInfo["LogicalProcessors"] = sysInfo.dwNumberOfProcessors;

        HRESULT hres;
        IWbemLocator* pLoc = NULL;
        IWbemServices* pSvc = NULL;

        hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres)) {
            std::cerr << "Falha ao inicializar COM.\n";
            return {};
        }

        hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        if (FAILED(hres)) {
            std::cerr << "Falha ao configurar segurança COM.\n";
            CoUninitialize();
            return {};
        }

        hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hres)) {
            std::cerr << "Falha ao criar instância WbemLocator.\n";
            CoUninitialize();
            return {};
        }

        hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hres)) {
            std::cerr << "Falha ao conectar ao servidor WMI.\n";
            pLoc->Release();
            CoUninitialize();
            return {};
        }

        hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
        if (FAILED(hres)) {
            std::cerr << "Falha ao definir contexto de segurança WMI.\n";
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return {};
        }

        IEnumWbemClassObject* pEnumerator = NULL;
        hres = pSvc->ExecQuery(bstr_t("WQL"),
            bstr_t("SELECT Name, MaxClockSpeed, CurrentClockSpeed FROM Win32_Processor"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL, &pEnumerator);

        if (FAILED(hres)) {
            std::cerr << "Falha ao executar consulta WMI.\n";
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return {};
        }

        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;

        // Lista de processadores
        nlohmann::json processors = nlohmann::json::array();

        while (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn) break;

            VARIANT vtName, vtMaxSpeed, vtCurSpeed;
            pclsObj->Get(L"Name", 0, &vtName, 0, 0);
            pclsObj->Get(L"MaxClockSpeed", 0, &vtMaxSpeed, 0, 0);
            pclsObj->Get(L"CurrentClockSpeed", 0, &vtCurSpeed, 0, 0);

            // Adiciona os dados do processador ao JSON
            processors.push_back({
                {"Name", _bstr_t(vtName.bstrVal)},
				{"MaxClockSpeedMHz", vtMaxSpeed.uintVal},
                {"CurrentClockSpeedMHz", vtCurSpeed.uintVal}
                });

            VariantClear(&vtName);
            VariantClear(&vtMaxSpeed);
            VariantClear(&vtCurSpeed);
            pclsObj->Release();
        }

        // Adiciona lista de processadores ao JSON principal
        cpuInfo["Processors"] = processors;

        pEnumerator->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();

        return cpuInfo;
	}
}