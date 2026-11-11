#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>

namespace {
std::wstring executableDirectory() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring result(path);
    return result.substr(0, result.find_last_of(L"\\/"));
}

bool runScene(int number) {
    if (number < 1 || number > 6) return false;
    std::wstring directory = executableDirectory();
    std::wstring program = directory + L"\\scene" + std::to_wstring(number) + L".exe";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(program.c_str(), nullptr, nullptr, nullptr, FALSE, 0,
                        nullptr, directory.c_str(), &startup, &process)) {
        std::wcerr << L"Failed to start " << program << L" (error " << GetLastError() << L")\n";
        return false;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    std::wcout << L"Scene " << number << L" exited with code " << exitCode << L".\n";
    return exitCode == 0;
}
} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc == 3 && std::wstring(argv[1]) == L"--scene") {
        int number = _wtoi(argv[2]);
        return runScene(number) ? 0 : 1;
    }
    std::wcout << L"Emporium functional reconstruction (CHAI3D)\n"
                  L"1  Microscopic World\n"
                  L"2  Haptic Textures\n"
                  L"3  Virtual Painting\n"
                  L"4  Deformable Membrane\n"
                  L"5  Dynamic Bodies\n"
                  L"6  Tactile Sensations\n"
                  L"0  Exit\n";
    while (true) {
        std::wcout << L"Scene number: " << std::flush;
        int number = 0;
        if (!(std::wcin >> number) || number == 0) break;
        if (!runScene(number)) std::wcerr << L"Scene did not finish successfully.\n";
    }
    return 0;
}
