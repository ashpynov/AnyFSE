#include "App/Application.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return AnyFSE::App::CallLibrary(
        _strcmpi(lpCmdLine, "/Service") ? L"AnyFSE.Control.dll" : L"AnyFSE.Service.dll",
        hInstance, hPrevInstance, lpCmdLine, nCmdShow
    );
}