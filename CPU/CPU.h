#ifndef CPU_H
#define CPU_H

#include "MonitorGraph.h"
#include <nlohmann/json.hpp>

namespace CPU {
    MONITORGRAPH_API double GetCPUUsage();
	MONITORGRAPH_API nlohmann::json GetCPUInfo();
}

#endif