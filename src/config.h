#ifndef LUNAPASSPORT_CONFIG_H
#define LUNAPASSPORT_CONFIG_H

#include <string>

struct PatchConfig {
    std::string ip;
    std::string domain;
    std::string passportHost;
    std::string memberservicesHost;

    bool BuildFromInput(const std::string& ipInput, const std::string& domainInput);
    std::wstring PreviewText() const;
};

#endif
