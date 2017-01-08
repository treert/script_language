#include<iostream>
#include<algorithm>
#include<iomanip>
#include<cstdio>
#include<string>
#include<vector>
#include<stdint.h>
#include<assert.h>
#include <fcntl.h>
#ifdef WIN32
#include <ws2tcpip.h>
#include <mstcpip.h>
//#include <winsock.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib") // 不然链接出错
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// 输出log
namespace {
    class  LogAddEnd
    {
    public:
        void operator=(std::ostream& other)
        {
            other << std::endl;
        }
    };
#define LOG LogAddEnd() = std::cout << __LINE__ << " " << "["<<__FUNCTION__<<"] "
#define LOG_NEW_LINE std::cout << std::endl
}

static int LastErrorNo()
{
#if WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

#if WIN32
static std::string LastErrorString()
{
    LPSTR pszError = NULL;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US), (LPSTR)&pszError, 0, NULL);

    std::string&& szErrorString = std::string(pszError);

    LocalFree(pszError);

    return szErrorString;
}

#else
static std::string LastErrorString()
{
    return std::string(strerror(LastErrorNo()));
}
#endif

#define LOG_NET_ERR_INFO LOG << "net_errno:" << LastErrorNo() << " msg:" << LastErrorString()

namespace NetUtils {

    void SleepSomeSeconds(int seconds)
    {
#ifdef WIN32
        Sleep(seconds * 1000);
#else
        sleep(seconds);
#endif // WIN32
    }

    // 在mac上没有效果哎，win32下有效
    std::string GetLocalIPAddress()
    {
        char ip[128] = { 0 };
        addrinfo hints;
        addrinfo *answer = nullptr,*curr = nullptr;
        
        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family = AF_INET; /* Allow IPv4 */
        hints.ai_protocol = 0; /* Any protocol */
        hints.ai_socktype = SOCK_STREAM;

        // mac上第二个参数传nullptr，报错。
        int ret = getaddrinfo("", "80", &hints, &answer);
        if (ret == 0)
        {
            for (curr = answer; curr != NULL; curr = curr->ai_next)
            {
                if (curr->ai_family == AF_INET)
                {
                    sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(curr->ai_addr);
                    inet_ntop(curr->ai_family, &addr->sin_addr, ip, sizeof(ip));
                    if (strcmp(ip, "127.0.0.1") == 0)
                    {
                        continue;
                    }
                }
                else if (curr->ai_family == AF_INET6)
                {
                    sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(curr->ai_addr);
                    inet_ntop(curr->ai_family, &addr->sin6_addr, ip, sizeof(ip));
                    if (strcmp(ip, "::1") == 0)
                    {
                        continue;
                    }
                }
                else
                {
                    LOG << "check it";
                    continue;
                }
                break;
            }
        }
        freeaddrinfo(answer);
        return ip;
    }

    std::string GetLocalIP(int socket_id)
    {
        sockaddr_storage local_addr;
        socklen_t len = sizeof(local_addr);

        if (getsockname(socket_id, (sockaddr*)&local_addr, &len) != 0)
        {
            LOG_NET_ERR_INFO;
            return "";
        }

        void* ip_ptr = nullptr;
        if (local_addr.ss_family == AF_INET)
        {
            ip_ptr = &((sockaddr_in *)&local_addr)->sin_addr;
        }
        else if (local_addr.ss_family == AF_INET6)
        {
            ip_ptr = &((sockaddr_in6 *)&local_addr)->sin6_addr;
        }
        else
        {
            return "";
        }

        char ip_str[128] = { 0 };
        auto* ptr = inet_ntop(local_addr.ss_family, ip_ptr, ip_str, sizeof(ip_str));
        if (ptr) {
            return ptr;
        }
        else
        {
            LOG_NET_ERR_INFO;
            return "";
        }
    }

    int GetLocalPort(int socket_id)
    {
        sockaddr_storage local_addr;
        socklen_t len;
        len = sizeof(local_addr);
        if (getsockname(socket_id, (sockaddr*)&local_addr, &len) != 0)
        {
            LOG_NET_ERR_INFO;
            return -1;
        }

        if (local_addr.ss_family == AF_INET)
        {
            sockaddr_in *addr4 = (sockaddr_in *)&local_addr;
            return ntohs(addr4->sin_port);
        }
        else if (local_addr.ss_family == AF_INET6)
        {
            sockaddr_in6 *addr6 = (sockaddr_in6 *)&local_addr;
            return  ntohs(addr6->sin6_port);
        }
        else
        {
            return -1;
        }
    }

    std::string GetPeerIP(int socket_id)
    {
        sockaddr_storage local_addr;
        socklen_t len = sizeof(local_addr);

        if (getpeername(socket_id, (sockaddr*)&local_addr, &len) != 0)
        {
            LOG << LastErrorNo() << LastErrorString();
            return "error: getsockname error";
        }

        void* ip_ptr = nullptr;
        if (local_addr.ss_family == AF_INET)
        {
            ip_ptr = &((sockaddr_in *)&local_addr)->sin_addr;
        }
        else if (local_addr.ss_family == AF_INET6)
        {
            ip_ptr = &((sockaddr_in6 *)&local_addr)->sin6_addr;
        }
        else
        {
            return "";
        }

        char ip_str[128] = { 0 };
        auto* ptr = inet_ntop(local_addr.ss_family, ip_ptr, ip_str, sizeof(ip_str));
        if (ptr) {
            return ptr;
        }
        else
        {
            LOG_NET_ERR_INFO;
            return "";
        }
    }

    int GetPeerPort(int socket_id)
    {
        sockaddr_storage local_addr;
        socklen_t len;
        len = sizeof(local_addr);
        if (getpeername(socket_id, (sockaddr*)&local_addr, &len) != 0)
        {
            LOG_NET_ERR_INFO;
            return -1;
        }

        if (local_addr.ss_family == AF_INET)
        {
            sockaddr_in *addr4 = (sockaddr_in *)&local_addr;
            return ntohs(addr4->sin_port);
        }
        else if (local_addr.ss_family == AF_INET6)
        {
            sockaddr_in6 *addr6 = (sockaddr_in6 *)&local_addr;
            return  ntohs(addr6->sin6_port);
        }
        else
        {
            return -1;
        }
    }

    static void CloseSocket(int socket_id)
    {
        if (socket_id != -1)
        {
#if WIN32
            closesocket(socket_id);
#else
            close(socket_id);
#endif
        }
    }

    static void SetKeepAlive(int socket_id, int idle_time, int interval_time)
    {
#if _WIN32
        struct StTcpKeepalive
        {
            uint32_t onoff;
            uint32_t keepalivetime;
            uint32_t keepaliveinterval;
        };
        StTcpKeepalive options;
        options.onoff = 1;
        options.keepalivetime = idle_time * 1000;
        options.keepaliveinterval = interval_time * 1000;
        DWORD cbBytesReturned;
        WSAIoctl(socket_id, SIO_KEEPALIVE_VALS, &options, sizeof(options), NULL, 0, &cbBytesReturned, NULL, NULL);
#else
        int keepAlive = 1;   // 开启keepalive属性. 缺省值: 0(关闭) 
        int keepIdle = idle_time;   // 如果在idle秒内没有任何数据交互,则进行探测. 缺省值:7200(s) 
        int keepInterval = interval_time;   // 探测时发探测包的时间间隔为interval秒. 缺省值:75(s) 
        int keepCount = 2;   // 探测重试的次数. 全部超时则认定连接失效..缺省值:9(次) 
        setsockopt(socket_id, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));
#ifdef __linux__
        setsockopt(socket_id, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
#endif
        setsockopt(socket_id, IPPROTO_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));
        setsockopt(socket_id, IPPROTO_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));
#endif
    }

    bool SetNoBlock(int fd)
    {
#ifdef WIN32
        unsigned long flag = 1;
        if (ioctlsocket(fd, FIONBIO, (unsigned long *)&flag) < 0)
        {
            LOG << "ioctlsocket set fail";
            return false;
        }
        return true;
#else
        int flags;
        if ((flags = fcntl(fd, F_GETFL)) == -1)
        {
            LOG << "fcntl get fail";
            return false;
        }

        flags |= O_NONBLOCK; //设置非阻塞标志位
        if (fcntl(fd, F_SETFL, flags) == -1)
        {
            LOG << "fcntl set fail";
            return false;
        }
        return true;
#endif
    }

    bool IsAlive(int socket_id) {
        int type = 0;
        socklen_t typesize = sizeof(type);
        int iCode = getsockopt(socket_id, SOL_SOCKET, SO_ERROR, (char*)&type, &typesize);
        return (iCode == 0);
    }

    // @params sock_type
    // TCP : SOCK_STREAM
    // UDP : SOCK_DGRAM
    int Connect(const std::string hostname_or_ip, uint16_t port, int sock_type)
    {
        addrinfo *answer, hint, *curr;
        int ret;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_UNSPEC;// 也可以用AF_INET或AF_INET6指定ipv4或ipv6
        hint.ai_socktype = sock_type;

        if ((ret = getaddrinfo(hostname_or_ip.c_str(), std::to_string(port).c_str(), &hint, &answer)) != 0) {
            LOG << "getaddrinfo fail errno:" << LastErrorNo() << " errstr:" << LastErrorString()
                << " extra_info:" << gai_strerror(ret);
            return -1;
        }

        sockaddr_in  sockaddr_ipv4;
        sockaddr_in6 sockaddr_ipv6;

        bool bValid = false;
        bool bIpv6Flag = false;
        for (curr = answer; curr != NULL; curr = curr->ai_next)
        {
            switch (curr->ai_family)
            {
            case AF_INET:
            {
                auto* sockaddr_ipv4_ptr = reinterpret_cast<sockaddr_in *>(curr->ai_addr);
                memset(&sockaddr_ipv4, 0, sizeof(sockaddr_ipv4));
                sockaddr_ipv4.sin_family = curr->ai_family;
                sockaddr_ipv4.sin_addr = sockaddr_ipv4_ptr->sin_addr;
                sockaddr_ipv4.sin_port = htons(port);
                bValid = true;
            }
            break;
            case AF_INET6:
            {
                auto* sockaddr_ipv6_ptr = reinterpret_cast<sockaddr_in6 *>(curr->ai_addr);
                memset(&sockaddr_ipv6, 0, sizeof(sockaddr_ipv6));
                sockaddr_ipv6.sin6_family = curr->ai_family;
                sockaddr_ipv6.sin6_addr = sockaddr_ipv6_ptr->sin6_addr;
                sockaddr_ipv6.sin6_port = htons(port);
                bValid = true;
                bIpv6Flag = true;
            }
            break;
            }
            if (bValid)
            {
                break;
            }
        }
        freeaddrinfo(answer);

        if (!bValid)
        {
            LOG << "getaddrinfo does not get valid sock_addr";
            return -1;
        }

        int addr_af = bIpv6Flag ? AF_INET6 : AF_INET;
        sockaddr * addr_ptr = bIpv6Flag ? (sockaddr *)&sockaddr_ipv6 : (sockaddr *)&sockaddr_ipv4;
        int addr_len = bIpv6Flag ? sizeof(sockaddr_ipv6) : sizeof(sockaddr_ipv4);

        int socket_id = socket(addr_af, sock_type, 0);
        if (socket_id < 0) {
            LOG_NET_ERR_INFO;
            return -1;
        }

        if (connect(socket_id, addr_ptr, addr_len) < 0) {
            LOG_NET_ERR_INFO;
            CloseSocket(socket_id);
            return -1;
        }

        SetNoBlock(socket_id);
        
        // TCP
        if(sock_type == SOCK_STREAM)
        {
            const int on = 1;
            setsockopt(socket_id, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
        }

#ifndef WIN32 // 信号会引起崩溃
        {
            const int set = 1;
            setsockopt(socket_id, SOL_SOCKET, SO_NOSIGPIPE, (const void *)&set, sizeof(set));
        }
#endif
        return socket_id;
    }



    int Listen(uint16_t port, int sock_type, std::string _bindAddress = "")
    {
        const int on = 1;
        int ret;
        int socket_id = -1;

        sockaddr_in  sockaddr_ipv4;
        memset(&sockaddr_ipv4, 0, sizeof(sockaddr_ipv4));
        sockaddr_ipv4.sin_family = AF_INET;
        sockaddr_ipv4.sin_addr.s_addr = INADDR_ANY;
        sockaddr_ipv4.sin_port = htons(port);
        // bind address
        if (_bindAddress.length() > 0)
        {
            inet_pton(AF_INET, _bindAddress.c_str(), (void*)&sockaddr_ipv4.sin_addr);
        }

        socket_id = socket(AF_INET, sock_type, 0);
        if (socket_id < 0)
        {
            LOG_NET_ERR_INFO;
            return -1;
        }
        // 重复使用本地址与socket文件进行绑定
        // 如果不设置系统，会保留此连接直到最后一引用才释放，进程结束后系统需要几分钟后才能重新进行绑定
        setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
        SetNoBlock(socket_id);
        ret = bind(socket_id, (struct sockaddr*)&sockaddr_ipv4, sizeof(sockaddr_ipv4));
        if (ret != 0)
        {
            LOG_NET_ERR_INFO;
            CloseSocket(socket_id);
            return -1;
        }

        listen(socket_id, 50);

        return socket_id;
    }

    int Accept(int socket_id)
    {
        sockaddr_storage client;
        socklen_t client_len = sizeof(client);

        /* new client */
        int fd = accept(socket_id, (struct sockaddr *)&client, &client_len);
        if (fd)
        {
            SetNoBlock(fd);
        }
        return fd;
    }


}



// 定义了个简单报文协议，开头两个字节的长度，| len(2) | data |
// 主要用于TCP
class CodeRingBuf {
public:
    CodeRingBuf()
    {
        _head = 0;
        _tail = 0;
    }

    int GetUsedBufLength()
    {
        return _tail - _head;
    }

    int GetFreeBufLength()
    {
        return BUF_REAL_LEN - GetUsedBufLength();
    }
    
    void Clear()
    {
        _head = _tail = 0;
    }

    // 输出缓存相关函数
    bool CheckFreeBufEnough(int len)
    {
        return GetFreeBufLength() >= len;
    }

    inline void _PutInOneChar(uint8_t ch)
    {
        _buf[_tail&BUF_REAL_LEN] = ch;
        ++_tail;
    }

    void PutInOneCode(const char* buf, int len)
    {
        len += 2;// 加上两个长度字节
        assert(GetFreeBufLength() >= len);
        _PutInOneChar((len >> 8) & 0xff);
        _PutInOneChar(len & 0xff);
        len -= 2;
        for (int i = 0; i < len; ++i)
        {
            _PutInOneChar(buf[i]);
        }
    }

    int CopyInfo(char* out_buf, int len) {
        int copy_len = GetUsedBufLength();
        if (len < copy_len) copy_len = len;

        for (int i = 0; i < copy_len; ++i)
        {
            out_buf[i] = _buf[(_head + i)&BUF_REAL_LEN];
        }
        return copy_len;
    }

    void Consume(int len)
    {
        assert(GetUsedBufLength() >= len);
        _head += len;
    }

    void PutIn(const char* buf, int len)
    {
        assert(GetFreeBufLength() >= len);
        for (int i = 0; i < len; ++i)
        {
            _PutInOneChar(buf[i]);
        }
    }

    int GetOutOneCode(char* out_buf, int buf_len)
    {
        int used_buf = GetUsedBufLength();
        if (used_buf < 2)
        {
            return -1;
        }

        int len = _buf[_head&BUF_REAL_LEN] << 8;
        len |= _buf[(_head + 1)&BUF_REAL_LEN];
        if (len < 2 || len > BUF_REAL_LEN)
        {
            LOG << "protocol error len=" << len;
            Clear();
            return -1;
        }

        if (GetUsedBufLength() < len)
        {
            return -1;
        }

        len -= 2;
        if (buf_len < len) {
            assert(!"this situation should not happen");
            Clear();
            return -1;
        }
        _head += 2;
        for (int i = 0; i < len; ++i)
        {
            out_buf[i] = _buf[_head&BUF_REAL_LEN];
            ++_head;
        }
        return len;
    }

private:
    // 代码有优化
    // buf长度一定要是2^n，有效长度是2^n-1.
    // 如果总流量<2^32，_tail就是总流量
    const static int BUF_LEN = 1 << 16;
    const static int BUF_REAL_LEN = BUF_LEN - 1;
    uint8_t _buf[BUF_LEN];
    uint32_t _head;
    uint32_t _tail;
};

class NetConnect {
public:
    virtual ~NetConnect() {};

    virtual int Connect(const std::string hostname_or_ip, uint16_t port) = 0;
    virtual void SendOneCode(const char* msg, int len) = 0;
    virtual int RecvOneCode(char* buf, int len) = 0;

    // 测试用
    virtual void SendMsg(const std::string msg)
    {
        SendOneCode(msg.c_str(), msg.length());
    }
    virtual std::string RecvMsg() {
        char buf[1500] = {0};
        // 一定要读到一组数据才结束
        while (RecvOneCode(buf, sizeof(buf)) < 0);
        return buf;
    }
};

class TCPConnect :public NetConnect {
public:
    TCPConnect() :_socket_id(-1), _is_breaked_by_server(false) {}

    TCPConnect& operator=(int socket_id)
    {
        _socket_id = socket_id;
        _is_breaked_by_server = false;
        return *this;
    }

    void LogInfo(std::string head)
    {
        LOG << head
            << " sock = " << NetUtils::GetLocalIP(_socket_id)
            << ":" << NetUtils::GetLocalPort(_socket_id)
            << " peer = " << NetUtils::GetPeerIP(_socket_id)
            << ":" << NetUtils::GetPeerPort(_socket_id);
    }

    int GetSocketId()
    {
        return _socket_id;
    }

    int Connect(const std::string hostname_or_ip, uint16_t port)
    {
        _send_buf.Clear();
        _recv_buf.Clear();
        _socket_id = NetUtils::Connect(hostname_or_ip, port, SOCK_STREAM);
        return _socket_id;
    }

    void SendOneCode(const char* msg, int len)
    {
        if (_send_buf.CheckFreeBufEnough(len + 2))
        {
            _send_buf.PutInOneCode(msg, len);
        }
        else
        {
            LOG << "buf overflow";
            return;
        }
        SendBuf();
    }

    void SendBuf()
    {
        if (_send_buf.GetUsedBufLength() > 0)
        {
            char buf[1024];
            int send_len,success_len;
            while ((send_len = _send_buf.CopyInfo(buf, sizeof(buf))) > 0)
            {
                success_len = send(_socket_id, buf, send_len, 0);
                if (success_len < 0)
                {
                    // todo(check errno)
                    break;
                }
                _send_buf.Consume(success_len);
            }
        }
    }

    int RecvOneCode(char* buf, int len)
    {
        // first read buf
        {
            int code_len = _recv_buf.GetOutOneCode(buf, len);
            if (code_len >= 0) {
                return code_len;
            }
        }

        for (;;)
        {
            char buf[1024];
            int recv_len;
            recv_len = recv(_socket_id, buf, len, 0);
            if (recv_len > 0)
            {
                if (_recv_buf.CheckFreeBufEnough(recv_len))
                {
                    _recv_buf.PutIn(buf, recv_len);
                }
                else
                {
                    LOG << "buf overflow";
                    // todo(reset connect)
                    break;
                }
                
            }
            else if (recv_len == 0)
            {
                // TCP recv()==0, connect is breaked
                break;
            }
            else if (recv_len < 0)
            {
                // todo(check errno)
                break;
            }
        }

        // last read buf
        {
            int code_len = _recv_buf.GetOutOneCode(buf, len);
            if (code_len >= 0) {
                return code_len;
            }
        }

        // get one code fail
        return -1;
    }

private:
    int _socket_id;
    bool _is_breaked_by_server;
    CodeRingBuf _send_buf;
    CodeRingBuf _recv_buf;
};

class UDPConnect :public NetConnect {
public:
    UDPConnect():_socket_id(-1) {}

    UDPConnect& operator=(int socket_id)
    {
        _socket_id = socket_id;
        return *this;
    }
    UDPConnect(int socket_id) : _socket_id(socket_id) {}

    int GetSocketId()
    {
        return _socket_id;
    }

    void LogInfo(std::string head)
    {
        LOG << head
            << " sock = " << NetUtils::GetLocalIP(_socket_id)
            << ":" << NetUtils::GetLocalPort(_socket_id);
    }

    int Connect(const std::string hostname_or_ip, uint16_t port)
    {
        _socket_id = NetUtils::Connect(hostname_or_ip, port, SOCK_DGRAM);
        return _socket_id;
    }

    void SendOneCode(const char* msg, int len)
    {
        // UDP不用分包
        int success_len = send(_socket_id, msg, len, 0);
        if (success_len < 0)
        {
            // todo(check errno)
        }
    }

    int RecvOneCode(char* buf, int len)
    {
        int success_len = recv(_socket_id, buf, len, 0);
        if (success_len < 0)
        {
            // todo(check errno)
            return -1;
        }
        return success_len;
    }
private:
    int _socket_id;
};

class TCPListenConnect {
public:
    TCPListenConnect():_socket_id(-1){}

    void LogInfo()
    {
        LOG << "socket_id:" << _socket_id
            << " listen on " << NetUtils::GetLocalIP(_socket_id)
            << ":" << NetUtils::GetLocalPort(_socket_id);
    }

    int GetSocketId()
    {
        return _socket_id;
    }

    int Listen(uint16_t port, std::string bind_address = "")
    {
        _socket_id = NetUtils::Listen(port, SOCK_STREAM, bind_address);
        return _socket_id;
    }

    int Accept()
    {
        return NetUtils::Accept(_socket_id);
    }
private:
    int _socket_id;
};

class UDPListenConnect {
public:
    UDPListenConnect():_socket_id(-1){}

    void LogInfo()
    {
        LOG << "socket_id:" << _socket_id
            << " listen on " << NetUtils::GetLocalIP(_socket_id)
            << ":" << NetUtils::GetLocalPort(_socket_id);
    }

    int GetSocketId()
    {
        return _socket_id;
    }

    int Listen(uint16_t port, std::string bind_address = "")
    {
        _socket_id = NetUtils::Listen(port, SOCK_DGRAM, bind_address);
        return _socket_id;
    }

    // 测试函数，反射多少包才结束，
    void ReflectALL(int num)
    {
        sockaddr_storage client;
        socklen_t client_len = sizeof(client);
        char buf[1024];
        int recv_len;
        for (;;)
        {
            recv_len = recvfrom(_socket_id, buf, sizeof(buf), 0, (sockaddr*)&client, &client_len);
            if (recv_len >= 0)
            {
                sendto(_socket_id, buf, recv_len, 0, (sockaddr*)&client, client_len);
                --num;
            }
            else
            {
                // todo(check errno)
                if (num <= 0)
                    break;
            }
        }
    }

private:
    int _socket_id;
};





int main() {
#ifdef WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        LOG << LastErrorNo() << LastErrorString();
        return 0;
    };
#endif

    std::string local_ip = "127.0.0.1";
#ifdef WIN32
    LOG << "local ip = " << NetUtils::GetLocalIPAddress();
#endif
    
    LOG_NEW_LINE;
    LOG << "TCP no block test start";
    {
        TCPListenConnect listen_connect;
        listen_connect.Listen(8080);
        listen_connect.LogInfo();
        TCPConnect client_connect;
        client_connect.Connect(local_ip, 8080);
        int id;
        TCPConnect server_connect;
        for (;;)
        {
            id = listen_connect.Accept();
            if (id > 0)
            {
                server_connect = id;
                break;
            }
        }
        client_connect.LogInfo("client");
        server_connect.LogInfo("server");

        client_connect.SendMsg("hello");
        LOG << "server recv: " << server_connect.RecvMsg();
        server_connect.SendMsg("hi");
        LOG << "client recv: " << client_connect.RecvMsg();
    }

    LOG_NEW_LINE;
    LOG << "UDP no block test start";
    {
        UDPListenConnect listen_connect;
        listen_connect.Listen(8080);
        listen_connect.LogInfo();
        UDPConnect client_connect;
        client_connect.Connect(local_ip, 8080);

        client_connect.LogInfo("client");

        client_connect.SendMsg("hello");

        listen_connect.ReflectALL(1);
        LOG << "client reflect: " << client_connect.RecvMsg();
    }

#ifdef WIN32
    WSACleanup();
#endif // WIN32
}