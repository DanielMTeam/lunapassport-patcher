#ifndef LUNAPASSPORT_HOSTS_H
#define LUNAPASSPORT_HOSTS_H

#include "config.h"
#include <string>

bool PatchHostsFile(const PatchConfig& config, std::wstring& log);

#endif
