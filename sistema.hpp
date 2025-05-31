
#ifndef SISTEMA_HPP
#define SISTEMA_HPP

#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#else
#include <sys/sysinfo.h>
#include <fstream>
#endif

using namespace std;

#define MB (1024 * 1024)

inline string extrair_nome_arquivo(const string& caminho_completo) {
    size_t pos = caminho_completo.find_last_of("/\\");
    if (pos == string::npos) return caminho_completo;
    return caminho_completo.substr(pos + 1);
}

inline size_t get_pico_uso_memoria_mb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return info.PeakWorkingSetSize / (1024 * 1024);
#else
    ifstream status("/proc/self/status");
    string linha;
    while (getline(status, linha)) {
        if (linha.find("VmHWM:") == 0) {
            size_t kb;
            sscanf(linha.c_str(), "VmHWM: %lu kB", &kb);
            return kb / 1024;
        }
    }
    return 0;
#endif
}

inline size_t get_memoria_total_mb() {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return static_cast<size_t>(status.ullTotalPhys / MB);
#else
    struct sysinfo info;
    if (sysinfo(&info) != 0) return 1024;
    return info.totalram * info.mem_unit / MB;
#endif
}

inline size_t get_memoria_disponivel_mb() {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return static_cast<size_t>(status.ullAvailPhys / MB);
#else
    struct sysinfo info;
    if (sysinfo(&info) != 0) return 1024;
    return info.freeram * info.mem_unit / MB;
#endif
}

inline size_t get_tamanho_arquivo_mb(const std::string& caminho) {
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesExA(caminho.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return 0;
    }

    ULARGE_INTEGER size;
    size.HighPart = fileInfo.nFileSizeHigh;
    size.LowPart = fileInfo.nFileSizeLow;
    return static_cast<size_t>(size.QuadPart / MB);
#else
    struct stat stat_buf;
    if (stat(caminho.c_str(), &stat_buf) != 0) return 0;
    return static_cast<size_t>(stat_buf.st_size / MB);
#endif
}

inline string get_nome_cpu() {
#ifdef _WIN32
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        TCHAR nome_cpu[256];
        DWORD tamanho = sizeof(nome_cpu);
        if (RegQueryValueEx(hKey, TEXT("ProcessorNameString"), NULL, NULL, (LPBYTE)nome_cpu, &tamanho) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return string(nome_cpu, nome_cpu + _tcslen(nome_cpu));
        }
        RegCloseKey(hKey);
    }
    return "Desconhecido";
#else
    ifstream cpuinfo("/proc/cpuinfo");
    string linha;
    while (getline(cpuinfo, linha)) {
        if (linha.find("model name") != string::npos) {
            return linha.substr(linha.find(":") + 2);
        }
    }
    return "Desconhecido";
#endif
}

#endif // SISTEMA_HPP
