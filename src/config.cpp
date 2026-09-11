#include "config.h"

bool PatchConfig::BuildFromInput(const std::string& ipInput, const std::string& domainInput) {
    ip = ipInput;
    domain = domainInput;

    while (!domain.empty() && domain[0] == '.') {
        domain.erase(0, 1);
    }
    while (!domain.empty() && domain[domain.size() - 1] == '.') {
        domain.erase(domain.size() - 1, 1);
    }

    if (ip.empty() || domain.empty()) {
        return false;
    }

    passportHost = "passport-staging." + domain;
    memberservicesHost = "memberservices-staging." + domain;
    return true;
}
