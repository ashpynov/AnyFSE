#include <string>
#include <windows.h>
#include <wincrypt.h>
#include <filesystem>

#include <winrt/Windows.Foundation.h>
#include <winrt/windows.applicationmodel.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Management.Deployment.h>

#include "AppInstaller.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"


namespace AnyFSE
{
    static Logger log = LogManager::GetLogger("Installer");

    bool AppInstaller::IsDeveloperModeEnabled()
    {
        return Registry::ReadDWORD(
            L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock",
            L"AllowDevelopmentWithoutDevLicense",
            0) == 1;
    }

    void AppInstaller::EnableDeveloperMode(bool bEnable)
    {
        Registry::WriteDWORD(
            L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock",
            L"AllowDevelopmentWithoutDevLicense",
            bEnable ? 1 : 0);
    }

    bool AppInstaller::IsRootCertificateInstalled(const std::wstring &commonName)
    {
        HCERTSTORE hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            0,
            NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
            L"ROOT"
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

    bool AppInstaller::InstallRootCertificate(const std::wstring &certFilePath)
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

    bool AppInstaller::RemoveRootCertificate(const std::wstring &publisherCN)
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

        PCCERT_CONTEXT pCertContext = nullptr;
        bool removed = false;

        // Find and remove all certificates with the matching common name
        while ((pCertContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, publisherCN.c_str(), pCertContext)))
        {
            // Duplicate the context before deletion since CertDeleteCertificateFromStore will free it
            PCCERT_CONTEXT pCertToDelete = CertDuplicateCertificateContext(pCertContext);
        
            if (pCertToDelete)
            {
                if (CertDeleteCertificateFromStore(pCertToDelete))
                {
                    removed = true;
                }
                // Note: CertDeleteCertificateFromStore automatically frees pCertToDelete
            }
        }
        if (pCertContext)
        {
            CertFreeCertificateContext(pCertContext);
        }

        CertCloseStore(hStore, 0);
        return removed;
    }

    bool AppInstaller::IsPackageInstalled(const std::wstring &packageFamilyName)
    {
        winrt::Windows::Management::Deployment::PackageManager packageManager;

        auto packages = packageManager.FindPackages(packageFamilyName);
        return (bool)packages;
    }

    bool AppInstaller::InstallPackage(const std::wstring &packageFilePath, const std::wstring & packageFamilyName)
    {
        winrt::init_apartment(); // Initialize WinRT apartment for using WinRT APIs

        try
        {
            namespace fs = std::filesystem;
            namespace d = winrt::Windows::Management::Deployment;
            namespace f = winrt::Windows::Foundation;

            d::PackageManager packageManager;

            // auto packages = packageManager.FindPackages(packageFamilyName);
            // for (auto& package : packages)
            // {
            //     auto removing = packageManager.RemovePackageAsync(package.Id().FullName());
            //     auto removed = removing.get();
            // }

            f::Uri packageUri(fs::absolute(packageFilePath).wstring());
            std::wcerr << packageUri.ToString().c_str() << std::endl;

            d::AddPackageOptions options;
            options.ForceUpdateFromAnyVersion(true);

            // Start the installation
            auto operation = packageManager.AddPackageByUriAsync(packageUri, options);

            // Wait for installation to complete
            auto result = operation.get();

            if (result.IsRegistered())
            {
                return true;
            }
            else
            {
                throw std::exception(Unicode::to_string(result.ErrorText().c_str()).c_str());
            }

            return true;
        }
        catch (const winrt::hresult_error &e)
        {
            throw std::exception(Unicode::to_string(e.message().c_str()).c_str());
        }
    }
};