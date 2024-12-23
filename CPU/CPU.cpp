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

// TODO: Serializar os dados para um JSON utilizando a lib nlohmann/json
// Usar o Logger aqui nas fun��es para registrar informa��es

namespace CPU {
    // Fun��o para coletar uso geral da CPU
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

        // Coleta dados pela primeira vez (necess�rio para inicializar o contador)
        PdhCollectQueryData(query);
        Sleep(1000); // Aguarda 1 segundo para obter um valor v�lido
        PdhCollectQueryData(query);

        // Obt�m o valor formatado do contador
        if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &counterValue) != ERROR_SUCCESS) {
            std::cerr << "Falha ao formatar valor do contador.\n";
            PdhCloseQuery(query);
            return -1;
        }

        // Fecha a consulta
        PdhCloseQuery(query);

        return counterValue.doubleValue;
    }

    // Fun��o para obter informa��es sobre n�cleos e threads
    MONITORGRAPH_API void GetCPUInfo() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        std::cout << "N�mero de Processadores L�gicos: " << sysInfo.dwNumberOfProcessors << "\n";

        // Conex�o com WMI
        HRESULT hres;
        IWbemLocator* pLoc = NULL;
        IWbemServices* pSvc = NULL;

        // Inicializa COM
        hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres)) {
            std::cerr << "Falha ao inicializar COM.\n";
            return;
        }

        // Configura seguran�a
        hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        if (FAILED(hres)) {
            std::cerr << "Falha ao configurar seguran�a COM.\n";
            CoUninitialize();
            return;
        }

        // Obt�m o localizador WMI
        hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hres)) {
            std::cerr << "Falha ao criar inst�ncia WbemLocator.\n";
            CoUninitialize();
            return;
        }

        // Conecta-se ao namespace WMI
        hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hres)) {
            std::cerr << "Falha ao conectar ao servidor WMI.\n";
            pLoc->Release();
            CoUninitialize();
            return;
        }

        // Define contexto de seguran�a WMI
        hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
        if (FAILED(hres)) {
            std::cerr << "Falha ao definir contexto de seguran�a WMI.\n";
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return;
        }

        // Consulta frequ�ncias dos n�cleos
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
            return;
        }

        // Processa os resultados
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        while (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn) break;

            VARIANT vtName, vtMaxSpeed, vtCurSpeed;
            pclsObj->Get(L"Name", 0, &vtName, 0, 0);
            pclsObj->Get(L"MaxClockSpeed", 0, &vtMaxSpeed, 0, 0);
            pclsObj->Get(L"CurrentClockSpeed", 0, &vtCurSpeed, 0, 0);

			// Exibe informa��es do processador atual
			// Propriedades: vtName, vtMaxSpeed, vtCurSpeed
			//               Nome, Frequ�ncia M�xima e Frequ�ncia Atual

            std::wcout << "Processador: " << vtName.bstrVal
                << " | Max: " << vtMaxSpeed.uintVal << " MHz"
                << " | Atual: " << vtCurSpeed.uintVal << " MHz\n";

            VariantClear(&vtName);
            VariantClear(&vtMaxSpeed);
            VariantClear(&vtCurSpeed);
            pclsObj->Release();
        }

        // Libera recursos
        pEnumerator->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
	}
}