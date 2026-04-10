#ifdef _WIN32
#include "serial-win.h"
#include <windows.h>
#include "string_helper.h"
#include "unicode.h"

bool Serial::Impl::open(std::string port, int baudrate) {
    auto pipeAt = port.find('|');
    if (pipeAt != std::string::npos) {
        port = port.substr(0, pipeAt);
    }

    std::string full = "\\\\.\\" + port;

    h = CreateFileA(full.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL);

    if (h == INVALID_HANDLE_VALUE) return false;

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);

    if (!GetCommState(h, &dcb)) return false;

    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;

    if (!SetCommState(h, &dcb)) return false;

    COMMTIMEOUTS t = {0};
    t.ReadIntervalTimeout = 50;
    t.ReadTotalTimeoutConstant = 50;
    t.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &t);

    return true;
}

void Serial::Impl::close() {
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
}

bool Serial::Impl::write(const std::vector<uint8_t>& data) {
    DWORD written = 0;
    return WriteFile(h, data.data(), (DWORD)data.size(), &written, NULL);
}
int Serial::Impl::available() {
    COMSTAT status;
    DWORD errors;

    if (!ClearCommError(h, &errors, &status))
        return 0;

    return status.cbInQue;
}
std::vector<uint8_t> Serial::Impl::read(int maxBytes, int) {
    std::vector<uint8_t> buf(maxBytes);
    DWORD read = 0;

    if (!ReadFile(h, buf.data(), maxBytes, &read, NULL))
        return {};

    buf.resize(read);
    return buf;
}

// Basic COM listing
// std::vector<std::string> Serial::listPorts() {
//     std::vector<std::string> out;
//     for (int i = 1; i <= 32; i++) {
//         std::string name = "COM" + std::to_string(i);
//         std::string full = "\\\\.\\" + name;
// 
//         HANDLE h = CreateFileA(full.c_str(), 0, 0, NULL, OPEN_EXISTING, 0, NULL);
//         if (h != INVALID_HANDLE_VALUE) {
//             out.push_back(name);
//             CloseHandle(h);
//         }
//     }
//     return out;
// }


#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <vector>
#include <string>

#pragma comment(lib, "setupapi.lib")

std::vector<std::string> Serial::listPorts() {
    std::vector<std::string> out;

    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_PORTS,
        NULL,
        NULL,
        DIGCF_PRESENT
    );

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return out;

    SP_DEVINFO_DATA devInfo{};
    devInfo.cbSize = sizeof(devInfo);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfo); i++) {
        wchar_t friendlyName[256]{};
        wchar_t portName[256]{};

        // Get friendly name
        if (!SetupDiGetDeviceRegistryPropertyW(
            hDevInfo,
            &devInfo,
            SPDRP_FRIENDLYNAME,
            NULL,
            (PBYTE)friendlyName,
            sizeof(friendlyName),
            NULL))
            continue;

        // Open registry key to get COM port
        HKEY hKey = SetupDiOpenDevRegKey(
            hDevInfo,
            &devInfo,
            DICS_FLAG_GLOBAL,
            0,
            DIREG_DEV,
            KEY_READ
        );

        if (hKey == INVALID_HANDLE_VALUE)
            continue;

        DWORD type = 0;
        DWORD size = sizeof(portName);

        if (RegQueryValueExW(
            hKey,
            L"PortName",
            NULL,
            &type,
            (LPBYTE)portName,
            &size) == ERROR_SUCCESS) {
            std::string pi = Unicode::toUtf8String((const char16_t*)&portName[0])
                + std::string("=")
                + Unicode::toUtf8String((const char16_t*)&friendlyName[0])
                + "";
            out.push_back(pi);
        }

        RegCloseKey(hKey);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return out;
}

bool Serial::isSerialPath(const std::string& path) {
    std::string lc = Unicode::toLowerAscii(path.c_str());
    return StringHelper::strncmp(lc.c_str(), "com", 3) == 0 ;
}


#endif