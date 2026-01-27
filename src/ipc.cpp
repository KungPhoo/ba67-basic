#include "ipc.h"

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h> // for InetPton
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/wait.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
typedef int SOCKET;
#endif

#include "unicode.h"


#ifdef _DEBUG
    #define dprintf(...) printf(__VA_ARGS__)
#else
    #define dprintf(...) ((void)0)
#endif


#ifdef _WIN32
static SOCKET invalidSock() { return INVALID_SOCKET; }
#else
static int invalidSock() { return -1; }
#endif


/* ================= IPCIMP ================= */
class IPCIMP {
public:
    IPC::Options options;
    std::thread worker;
    std::atomic<bool> running { false };
    std::atomic<bool> flushed { false };

    std::vector<uint8_t> inBuf, outBuf;
    std::mutex inMutex, outMutex;

    int tcpPort = 0;

#ifdef _WIN32
    HANDLE hChild = NULL;
    HANDLE hInW = NULL, hOutR = NULL;
    SOCKET sock = INVALID_SOCKET;
#else
    pid_t childPid = -1;
    int inFd = -1, outFd = -1;
    int sock = -1;
#endif

    bool openPipe();
    void pipeThread();

    bool openTcp();
    SOCKET acceptTcp(SOCKET s);
    bool finishConnect(SOCKET s);
    void tcpThread();


    SOCKET connectTcp(int port);

    SOCKET listenTcp(int port);

    void closeSocket(SOCKET& sock);
    void closeHandles();
};



/* ================= PIPE ================= */
bool IPCIMP::openPipe() {
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa { sizeof(sa), NULL, TRUE };
    HANDLE inR, outW;

    CreatePipe(&inR, &hInW, &sa, 0);
    CreatePipe(&hOutR, &outW, &sa, 0);

    STARTUPINFO si {};
    si.cb         = sizeof(si);
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = inR;
    si.hStdOutput = outW;
    si.hStdError  = outW;

    std::u16string cmd_w;
    if (!Unicode::toU16String(options.programPath.c_str(), cmd_w)) {
        return false;
    }


    PROCESS_INFORMATION pi {};
    if (!CreateProcessW(NULL, LPWSTR(&cmd_w[0]), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {

        auto le = GetLastError();
        return false;
    }

    CloseHandle(inR);
    CloseHandle(outW);
    CloseHandle(pi.hThread);
    hChild = pi.hProcess;
#else
    int pin[2], pout[2];
    pipe(pin);
    pipe(pout);

    childPid = fork();
    if (childPid == 0) {
        dup2(pin[0], STDIN_FILENO);
        dup2(pout[1], STDOUT_FILENO);
        execl(options.programPath.c_str(), options.programPath.c_str(), nullptr);
        _exit(1);
    }

    close(pin[0]);
    close(pout[1]);
    inFd  = pin[1];
    outFd = pout[0];
#endif
    worker = std::thread(&IPCIMP::pipeThread, this);
    return true;
}

void IPCIMP::pipeThread() {
    uint8_t buf[256] = {};
    while (running) {
        // read
#ifdef _WIN32
        DWORD n = 0;
        if (ReadFile(hOutR, buf, sizeof(buf), &n, NULL) && n) {
            std::lock_guard<std::mutex> lock(inMutex);
            inBuf.insert(inBuf.end(), buf, buf + n);
        }
#else
        ssize_t n = ::read(outFd, buf, sizeof(buf));
        if (n > 0) {
            std::lock_guard<std::mutex> lock(inMutex);
            inBuf.insert(inBuf.end(), buf, buf + n);
        }
#endif

        // write
        if (flushed) {
            flushed = false;
            std::lock_guard<std::mutex> lock(outMutex);
#ifdef _WIN32
            DWORD w;
            WriteFile(hInW, outBuf.data(), (DWORD)outBuf.size(), &w, NULL);
#else
            ::write(inFd, outBuf.data(), outBuf.size());
#endif
            outBuf.clear();
        }
    }
}



/* ================= TCP ================= */

static void setNonBlocking(SOCKET s) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
#else
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#endif
}



bool IPCIMP::openTcp() {
    if (options.hostname.empty()) {
        options.hostname = "localhost";
    }

#ifdef _WIN32
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
#endif

    running = true;
    worker  = std::thread(&IPCIMP::tcpThread, this);
    return running;
}


SOCKET IPCIMP::acceptTcp(SOCKET s) {
    SOCKET a = accept(s, nullptr, nullptr);

#ifdef _WIN32
    if (a == INVALID_SOCKET && WSAGetLastError() == WSAEWOULDBLOCK) {
        return invalidSock();
    }
#else
    if (a < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return invalidSock();
    }
#endif

    setNonBlocking(a);
    return a;
}

bool IPCIMP::finishConnect(SOCKET s) {
    int err       = 0;
    socklen_t len = sizeof(err);
    getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
    return err == 0;
}

void IPCIMP::tcpThread() {
    SOCKET sock = invalidSock();

    enum class State { None,
                       Listening,
                       Connecting,
                       Connected };
    State state = State::None;

    const int bufsize        = 256;
    uint8_t buf[bufsize + 1] = {};

    while (running) {

        // cool the fans
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // -----------------------
        // Establish connection
        // -----------------------
        if (state == State::None) {
            dprintf("try connect to %s:%d...\n", options.hostname.c_str(), options.port);
            sock = connectTcp(options.port);

            if (sock != invalidSock()) {
                dprintf("connecting...\n");
                state = State::Connecting;
            } else {
                sock  = listenTcp(options.port);
                state = State::Listening;
                if (sock != invalidSock()) {
                    dprintf("listening...\n");
                } else {
                    dprintf("can't listen nor connect?\n");
                }
            }
            continue;
        }

        // -----------------------
        // Wait sets
        // -----------------------
        fd_set rset, wset; // for select() - check for read/write data available
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        SOCKET maxfd = sock;

        if (state == State::Connecting) {
            FD_SET(sock, &wset); // finish connect
        }

        if (state == State::Listening) {
            FD_SET(sock, &rset); // accept
        }

        if (state == State::Connected) {
            FD_SET(sock, &rset); // recv
            if (flushed) {
                FD_SET(sock, &wset); // send
            }
        }

        timeval tv { 0, 20000 }; // 20ms
        select(int(maxfd + 1), &rset, &wset, nullptr, &tv);



        // -----------------------
        // Finish connect
        // -----------------------
        if (state == State::Connecting && FD_ISSET(sock, &wset)) {

            if (finishConnect(sock)) {
                state = State::Connected;
                dprintf("finishConnect - connected\n");
            } else {
                closeSocket(sock);
                sock  = invalidSock();
                state = State::None;
            }
        }

        // -----------------------
        // Accept
        // -----------------------
        if (state == State::Listening && FD_ISSET(sock, &rset)) {

            SOCKET s = acceptTcp(sock);

            if (s != invalidSock()) {
                closeSocket(sock);
                sock  = s;
                state = State::Connected;
                dprintf("accept - connected\n");
            }
        }


        // -----------------------
        // Receive
        // -----------------------
        if (state == State::Connected && FD_ISSET(sock, &rset)) {
            int n = recv(sock, (char*)buf, bufsize, 0);

            if (n > 0) {
                buf[n] = '\0';
                dprintf("recv '%s'\n", buf);

                std::lock_guard<std::mutex> l(inMutex);
                inBuf.insert(inBuf.end(), buf, buf + n);
            } else if (n == 0) {
                // Peer closed connection
                dprintf("disconnected\n");
                state = State::None;
                closeSocket(sock);
                sock = invalidSock();
                continue;
            } else {
                // n < 0 : check error
#ifdef _WIN32
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK) {
                    dprintf("disconnected. error %d\n", err);
                    state = State::None;
                    closeSocket(sock);
                    sock = invalidSock();
                    continue;
                }
#else
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    dprintf("disconnected. error %d\n", err);
                    state = State::None;
                    closeSocket(sock);
                    sock = invalidSock();
                    continue;
                }
#endif
            }
        }

        // -----------------------
        // Send
        // -----------------------
        if (state == State::Connected && flushed && FD_ISSET(sock, &wset)) {

            std::lock_guard<std::mutex> l(outMutex);
            int n = send(sock,
                         (char*)outBuf.data(),
                         (int)outBuf.size(), 0);
            dprintf("sent %d/%d\n", n, int(outBuf.size()));


            outBuf.clear();
            flushed = false;
        }
    }

    // cleanup
    closeSocket(sock);
    dprintf("cleanup\n");
}


/* ================= Helpers ================= */


#ifdef _DEBUG
// helper for debugging
char* get_ip_str(const struct sockaddr* sa) {
    const int maxlen    = 1023;
    static char s[1024] = {};
    switch (sa->sa_family) {
    case AF_INET:
        inet_ntop(AF_INET,
                  &(((struct sockaddr_in*)sa)->sin_addr), s,
                  maxlen);
        break;

    case AF_INET6:
        inet_ntop(AF_INET6,
                  &(((struct sockaddr_in6*)sa)->sin6_addr), s,
                  maxlen);
        break;

    default:
        strncpy(s, "Unknown AF", maxlen);
        return NULL;
    }

    return s;
}
#endif


SOCKET IPCIMP::connectTcp(int port) {
    const int timeoutMs = 300;
    addrinfo hints {};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res       = nullptr;
    std::string portStr = std::to_string(port);

    if (getaddrinfo(options.hostname.c_str(),
                    portStr.c_str(),
                    &hints, &res)
        != 0) {
        return invalidSock();
    }

    SOCKET s = invalidSock();

    for (addrinfo* a = res; a; a = a->ai_next) {

#ifdef _DEBUG
        dprintf("%s resolves to %s\n",
                options.hostname.c_str(),
                get_ip_str(a->ai_addr));
#endif

        s = socket(a->ai_family,
                   a->ai_socktype,
                   a->ai_protocol);

        if (s == invalidSock()) {
            continue;
        }

        setNonBlocking(s);

        int rc = connect(s,
                         a->ai_addr,
                         (int)a->ai_addrlen);

        if (rc == 0) {
            // connected immediately (rare)
            break;
        }

        // Check "in progress"
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS)
#else
        if (errno != EINPROGRESS)
#endif
        {
            closeSocket(s);
            s = invalidSock();
            continue;
        }

        // -----------------------------
        // Wait for connection
        // -----------------------------

        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(s, &wset);

        timeval tv;
        tv.tv_sec  = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        rc = select(int(s + 1),
                    nullptr,
                    &wset,
                    nullptr,
                    &tv);

        if (rc <= 0) {
            // timeout or error
            closeSocket(s);
            s = invalidSock();
            continue;
        }

        // -----------------------------
        // Check result
        // -----------------------------

        int soerr     = 0;
        socklen_t len = sizeof(soerr);

        getsockopt(s,
                   SOL_SOCKET,
                   SO_ERROR,
                   (char*)&soerr,
                   &len);

        if (soerr == 0) {
            // connected!
            break;
        }

        // failed
        closeSocket(s);
        s = invalidSock();
    }

    freeaddrinfo(res);
    return s;
}


SOCKET IPCIMP::listenTcp(int port) {
    addrinfo hints {};
    hints.ai_family   = AF_UNSPEC; // BOTH
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    addrinfo* res       = nullptr;
    std::string portStr = std::to_string(port);

    if (getaddrinfo(nullptr, portStr.c_str(), &hints, &res) != 0) {
        return invalidSock();
    }

    SOCKET s = invalidSock();

    for (addrinfo* a = res; a; a = a->ai_next) {
        s = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (s == invalidSock()) {
            continue;
        }

        int yes = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

        if (bind(s, a->ai_addr, (int)a->ai_addrlen) == 0 && listen(s, 1) == 0) {
            break;
        }

        closeSocket(s);
        s = invalidSock();
    }

    freeaddrinfo(res);

    if (s != invalidSock()) {
        setNonBlocking(s);
    }

    return s;
}

void IPCIMP::closeSocket(SOCKET& sock) {
    if (sock == invalidSock()) {
        return;
    }
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    sock = invalidSock();
}

void IPCIMP::closeHandles() {
    closeSocket(sock);

#ifdef _WIN32
    if (hInW) {
        CloseHandle(hInW);
        hInW = NULL;
    }
    if (hOutR) {
        CloseHandle(hOutR);
        hOutR = NULL;
    }
    if (hChild) {
        CloseHandle(hChild);
        hChild = NULL;
    }
    WSACleanup();
#else
    if (inFd != -1) {
        close(inFd);
        inFd = -1;
    }
    if (outFd != -1) {
        close(outFd);
        outFd = -1;
    }
    if (childPid > 0) {
        waitpid(childPid, nullptr, 0);
        childPid = 0;
    }
#endif
}



/* ================= IPC ================= */
bool IPC::open(const IPC::Options& opt) {
    close();
    imp->options = opt;

    if (imp->options.mode == IPC_MODE::TCP) {
        return imp->openTcp();
    }
    return imp->openPipe();
}


void IPC::close() {
    imp->running = false;
    if (imp->worker.joinable()) {
        imp->worker.join();
    }
    imp->closeHandles();
}

void IPC::putc(uint8_t b) {
    std::lock_guard<std::mutex> lock(imp->outMutex);
    imp->outBuf.push_back(b);
}

void IPC::puts(const std::string& str) {
    std::lock_guard<std::mutex> lock(imp->outMutex);

    imp->outBuf.reserve(imp->outBuf.size() + str.length());
    for (char c : str) {
        imp->outBuf.push_back(uint8_t(c));
    }
}

void IPC::flush() {
    imp->flushed = true;
}

bool IPC::hasData() {
    std::lock_guard<std::mutex> lock(imp->inMutex);
    return !imp->inBuf.empty();
}

uint8_t IPC::getc() {
    std::lock_guard<std::mutex> lock(imp->inMutex);
    uint8_t b = imp->inBuf.front();
    imp->inBuf.erase(imp->inBuf.begin());
    return b;
}

std::string IPC::gets() {
    std::lock_guard<std::mutex> lock(imp->inMutex);

    std::string str;
    str.reserve(imp->inBuf.size() + 1);

    for (uint8_t i : imp->inBuf) {
        str += char(i);
    };
    imp->inBuf.clear();
    return str;
}

IPC::IPC() { imp = new IPCIMP(); }

IPC::~IPC() {
    delete imp;
    imp = nullptr;
}
bool IPC::isRunning() const { return imp->running; }
