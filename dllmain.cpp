#include "pch.h"
// dllmain.cpp : Defines the entry point for the DLL application.
#include <iostream>

#include "MonitorGraph.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        //Quando for anexado a dll em algum processo vai ser executado o que eu colocar aqui embaixo
        //Por exemplo chamar uma função de dentro da biblioteca do monitor graph
        //ExampleFunction();
        std::cout << "MonitorGraph DLL is working!" << std::endl;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
	return TRUE;
}