// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include <windows.h>

namespace AnyFSE::App::CmdLine
{
    bool AsHidListener(LPSTR lpCmdLine);
    bool AsHidListenerJob(LPSTR lpCmdLine);
    bool AsElevated(LPSTR lpCmdLine);
    bool AsFSE(LPSTR lpCmdLine);
    bool AsFSENow(LPSTR lpCmdLine);
    bool AsFSEReboot(LPSTR lpCmdLine);
    bool AsSettings(LPSTR lpCmdLine);

    bool Elevated(LPSTR lpCmdLine, int &result);
    bool HidListenerJob(LPSTR lpCmdLine, int &result);
    bool HidListener(LPSTR lpCmdLine, int &result);
    bool FSE(LPSTR lpCmdLine, int &result);
    bool Settings(LPSTR lpCmdLine, int &result);
};