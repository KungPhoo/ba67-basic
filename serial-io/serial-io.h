#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Serial {
public:
    Serial();
    ~Serial();

    bool open(std::string port, int baudrate = 115200);
    void close();

    bool write(const std::vector<uint8_t>& data);
    int available(); // number of bytes ready
    std::vector<uint8_t> read(int maxBytes = 512, int timeoutMs = -1);

    bool isOpen() const;

    // returns strings like "COM5=Standard Serial over Bluetooth link (COM5)"
    // or just "COM5"
    // or "/dev/tty1"
    static std::vector<std::string> listPorts();
    static bool isSerialPath(const std::string& path);

private:
    struct Impl;
    Impl* impl;
};