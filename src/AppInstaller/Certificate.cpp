#include <windows.h>
#include <filesystem>
#include <string>

namespace AnyFSE::ToolsEx::Certificate
{
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
        return IsCertificateInstalled(commonName, L"ROOT");
    }

    bool InstallRootCertificate(const std::wstring &certFilePath)
    {
        HCERTSTORE hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            0,
            NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            L"ROOT"
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

        bool result = CertAddCertificateContextToStore(hStore, pCertContext, CERT_STORE_ADD_REPLACE_EXISTING, NULL);
        CertFreeCertificateContext(pCertContext);
        CertCloseStore(hStore, 0);

        return result;
    }

    bool RemoveRootCertificate(const std::wstring &publisherCN)
    {
        return RemoveCertificate(publisherCN, L"ROOT");
    }
}
