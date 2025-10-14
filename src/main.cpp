#include "Application.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // If failed as service, run as normal application
    return AnyFSE::Application::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}