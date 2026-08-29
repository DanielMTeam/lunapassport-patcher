#include "config.h"
#include "i18n.h"
#include "util.h"

#include <sstream>

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

std::wstring PatchConfig::PreviewText() const {
    std::wstringstream ss;
    ss << Tr(STR_PREVIEW_PASSPORT) << Utf8ToWide(passportHost) << L"\r\n"
       << Tr(STR_PREVIEW_MEMBERSERVICES) << Utf8ToWide(memberservicesHost) << L"\r\n"
       << Tr(STR_PREVIEW_HOSTS) << Utf8ToWide(ip) << L" + register.passport.com";
    return ss.str();
}
