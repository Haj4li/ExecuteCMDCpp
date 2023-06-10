#include <windows.h>
#include <iostream>

using namespace std;

// https://github.com/Haj4li

HANDLE hRead, hWrite, hWrite2, hRead2;
PROCESS_INFORMATION procInfo;
STARTUPINFOA startInfo;

bool InitilizePipes();
void Dispose();
string executeCommand(string command);

int main() {
    InitilizePipes(); // handles and pipes must be initilized once

    string command = "dir";
    string result = executeCommand(command);
    cout << result << endl;

    Dispose(); // keep in mind to close process and handles before the program terminates
    return 0;
}

bool InitilizePipes()
{
    DWORD stuff;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, true };
    ZeroMemory(&procInfo, sizeof(procInfo));
    ZeroMemory(&startInfo, sizeof(startInfo));

    if (!CreatePipe(&hRead, &hWrite, &sa, 0) || !CreatePipe(&hRead2, &hWrite2, &sa, 0)) {
        return false;
    }

    startInfo.cb = sizeof(STARTUPINFOA);
    startInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startInfo.wShowWindow = SW_HIDE;
    startInfo.hStdInput = hRead2;
    startInfo.hStdOutput = hWrite;
    startInfo.hStdError = hWrite;
    char windows[MAX_PATH];
    if (!GetWindowsDirectoryA(windows, MAX_PATH)) {
        CloseHandle(&startInfo);
        CloseHandle(&procInfo);
        return false;
    }
    std::string win_path = windows;
    win_path += "\\System32\\cmd.exe";
    if (!CreateProcessA(win_path.c_str(), NULL, NULL, NULL, TRUE, 0, NULL, NULL, &startInfo, &procInfo)) {
        CloseHandle(&startInfo);
        CloseHandle(&procInfo);
        return false;
    }

    GetExitCodeProcess(procInfo.hProcess, &stuff);
    if (stuff != STILL_ACTIVE) {
        CloseHandle(&startInfo);
        CloseHandle(&procInfo);
        return false;
    }
    // ....
    return true;
}

void Dispose()
{
    TerminateProcess(procInfo.hProcess, 0);
    WaitForSingleObject(procInfo.hProcess, 10 * 1000);
    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);
}

string executeCommand(string command) {
    std::string result = "";

    DWORD bytes = 0;
    std::string tmp = command;

    tmp += "\r\n";
    OVERLAPPED overlapped = { 0 };
    DWORD bytesTransferred;
    while (!GetOverlappedResult(hWrite2, &overlapped, &bytesTransferred, false))
    {
        DWORD error = GetLastError();
        if (error == ERROR_IO_INCOMPLETE || error == ERROR_IO_PENDING) {
            Sleep(5000);
        }
    }
    WriteFile(hWrite2, tmp.c_str(), tmp.length(), &bytes, NULL);
    DWORD size;

    Sleep(3500);

    std::string str = "";
    char* buffer = NULL;

    while (1)
    {
        PeekNamedPipe(hRead, NULL, NULL, NULL, &size, NULL);

        if (size)
        {
            buffer = (char*)malloc(size + 1);
            memset(buffer, 0, static_cast<size_t>(size) + 1);
            ReadFile(hRead, buffer, size, &bytes, NULL);
        }

        if (bytes > 0)
        {
            str += buffer;
            free(buffer);
        }
        else
        {
            if (size)
            {
                free(buffer);
            }

            break;
        }
        bytes = 0;
        Sleep(1000);
    }
    return str;
}
