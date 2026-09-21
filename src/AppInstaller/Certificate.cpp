#include <windows.h>
#include <filesystem>
#include <string>
#include "Certificate.hpp"
#include "App/Constants.hpp"

namespace AnyFSE::ToolsEx::Certificate
{
    namespace c = AnyFSE::App::Constants;
    namespace
    {
    bool IsCertificateInstalled(const std::wstring &commonName, const wchar_t *storeName)
    {
        HCERTSTORE hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            0,
            NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
            storeName
        );
        if (!hStore)
            return false;

        PCCERT_CONTEXT pCertContext = nullptr;
        bool found = false;

        while ((pCertContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, commonName.c_str(), pCertContext)))
        {
            found = true; // Certificate found
            CertFreeCertificateContext(pCertContext);
            break;
        }

        CertCloseStore(hStore, 0);
        return found;
    }

    bool RemoveCertificate(const std::wstring &publisherCN, const wchar_t *storeName)
    {
        HCERTSTORE hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            0,
            NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            storeName
        );

        if (!hStore)
        {
            return false;
        }

        PCCERT_CONTEXT pCertContext = nullptr;
        bool removed = false;

        while ((pCertContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, publisherCN.c_str(), pCertContext)))
        {
            PCCERT_CONTEXT pCertToDelete = CertDuplicateCertificateContext(pCertContext);
            if (pCertToDelete && CertDeleteCertificateFromStore(pCertToDelete))
            {
                removed = true;
            }
        }

        CertCloseStore(hStore, 0);
        return removed;
    }
    }

    bool IsRootCertificateInstalled(const std::wstring &commonName)
    {
        return IsCertificateInstalled(commonName, c::RootCertificateStore);
    }

    bool IsTrustedPeopleCertificateInstalled(const std::wstring &commonName)
    {
        return IsCertificateInstalled(commonName, c::TrustedPeopleCertificateStore);
    }

    bool RemoveTrustedPeopleCertificate(const std::wstring &publisherCN)
    {
        return RemoveCertificate(publisherCN, c::TrustedPeopleCertificateStore);
    }

    bool InstallTrustedPeopleCertificate(const std::wstring &certFilePath)
    {
        HCERTSTORE hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            0,
            NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            c::TrustedPeopleCertificateStore
        );

        if (!hStore)
        {
            return false;
        }

        HANDLE hFile = CreateFile(certFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            CertCloseStore(hStore, 0);
            return false;
        }

        DWORD fileSize = GetFileSize(hFile, NULL);
        BYTE *buffer = new BYTE[fileSize];
        DWORD bytesRead;

        if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL))
        {
            CloseHandle(hFile);
            delete[] buffer;
            CertCloseStore(hStore, 0);
            return false;
        }

        CloseHandle(hFile);

        PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING, buffer, fileSize);
        delete[] buffer;

        if (!pCertContext)
        {
            CertCloseStore(hStore, 0);
            return false;
        }

        PCCERT_CONTEXT storedCert = nullptr;
        bool result = CertAddCertificateContextToStore(hStore, pCertContext, CERT_STORE_ADD_REPLACE_EXISTING, &storedCert);
        if (result)
        {
            std::wstring friendlyName(c::PublisherCertFriendlyName);
            CRYPT_DATA_BLOB name{};
            name.cbData = static_cast<DWORD>((friendlyName.size() + 1) * sizeof(wchar_t));
            name.pbData = reinterpret_cast<BYTE*>(friendlyName.data());
            result = CertSetCertificateContextProperty(storedCert, CERT_FRIENDLY_NAME_PROP_ID, 0, &name);
            CertFreeCertificateContext(storedCert);
        }
        CertFreeCertificateContext(pCertContext);
        CertCloseStore(hStore, 0);

        return result;
    }

    bool RemoveRootCertificate(const std::wstring &publisherCN)
    {
        return RemoveCertificate(publisherCN, c::RootCertificateStore);
    }
}
