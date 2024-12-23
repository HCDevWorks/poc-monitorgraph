#ifndef CPU_H
#define CPU_H

#include "MonitorGraph.h"

namespace CPU {
    MONITORGRAPH_API double GetCPUUsage();
	MONITORGRAPH_API void GetCPUInfo();
}

#endif