#pragma once

#include <string>

class IPCIMP;

class IPC {
public:
    enum class IPC_MODE { PIPE,
                          TCP };


    struct Options {
        IPC_MODE mode = IPC_MODE::PIPE;
        std::string programPath;

        // for mode PIPE: what pipes to connect to
        // bool stdIn  = true;
        // bool stdOut = true;
        // bool stdErr = false;

        // for mode TCP:
        // If portIn == portOut, IPC opens a single bidirectional TCP connection.
        // The first process becomes the server, the second becomes the client.
        std::string hostname = "localhost";
        int port             = 0;
    };

    IPC();
    ~IPC();

    // start program and connect to it
    bool open(const Options& opt);

    // check if background thread is active
    bool isRunning() const;

    // close connection
    void close();

    // apped byte to output cache
    void putc(uint8_t b);

    // write string to output cache
    void puts(const std::string& str);

    // caching chout finished. Allow to send data now.
    void flush();

    // input cache has data?
    bool hasData();

    // input from input cache
    uint8_t getc();

    // get all from input cache
    std::string gets();

private:
    IPCIMP* imp;
};