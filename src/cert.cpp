#include "cert.h"
#include "util.h"

#include "resource.h"

#include <vector>
#include <wincrypt.h>

static bool LoadEmbeddedCert(std::vector<BYTE>& data) {
    HMODULE module = GetModuleHandleW(NULL);
    HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(IDR_ISRG_ROOT_X1), RT_RCDATA);
    if (!resource) {
        return false;
    }

    HGLOBAL loaded = LoadResource(module, resource);
    if (!loaded) {
        return false;
    }

    DWORD size = SizeofResource(module, resource);
    void* bytes = LockResource(loaded);
    if (!bytes || size == 0) {
        return false;
    }

    data.assign(static_cast<BYTE*>(bytes), static_cast<BYTE*>(bytes) + size);
    return true;
}

static bool CertAlreadyInstalled(const BYTE* data, DWORD size) {
    PCCERT_CONTEXT context =
        CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, data, size);
    if (!context) {
        return false;
    }

    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, NULL,
                                     CERT_SYSTEM_STORE_LOCAL_MACHINE, L"ROOT");
    if (!store) {
        CertFreeCertificateContext(context);
        return false;
    }

    PCCERT_CONTEXT existing = CertFindCertificateInStore(
        store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_EXISTING, context, NULL);

    bool found = existing != NULL;
    if (existing) {
        CertFreeCertificateContext(existing);
    }

    CertCloseStore(store, 0);
    CertFreeCertificateContext(context);
    return found;
}

bool ImportLetsEncryptRoot(std::wstring& log) {
    std::vector<BYTE> data;
    if (!LoadEmbeddedCert(data)) {
        log += L"[ERR] embedded CA resource missing\r\n";
        return false;
    }

    if (CertAlreadyInstalled(&data[0], static_cast<DWORD>(data.size()))) {
        log += L"[OK] Let's Encrypt root already installed\r\n";
        return true;
    }

    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, NULL,
                                     CERT_SYSTEM_STORE_LOCAL_MACHINE, L"ROOT");
    if (!store) {
        log += L"[ERR] cannot open ROOT store\r\n";
        return false;
    }

    BOOL added = CertAddEncodedCertificateToStore(
        store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &data[0],
        static_cast<DWORD>(data.size()), CERT_STORE_ADD_USE_EXISTING, NULL);

    CertCloseStore(store, 0);

    if (!added) {
        DWORD error = GetLastError();
        log += L"[ERR] CA import failed: " + FormatWin32Error(error) + L"\r\n";
        return false;
    }

    log += L"[OK] Let's Encrypt root imported\r\n";
    return true;
}
