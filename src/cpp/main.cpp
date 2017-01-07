#include<iostream>
#include<algorithm>
#include<iomanip>
#include<cstdio>
#include<string>
#include<vector>
#include<stdint.h>
#include<assert.h>
#ifdef WIN32
#include <ws2tcpip.h>
#include <mstcpip.h>
//#include <winsock.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib") // ��Ȼ���ӳ���
#include <windows.h>
#elif defined(LINUX)
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

// ���log
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

    std::string GetLocalIPAddress()
    {
        char ip[16] = { 0 };
        addrinfo hints;
        addrinfo *res = nullptr;
        sockaddr_in *addr;

        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family = AF_INET; /* Allow IPv4 */
        hints.ai_protocol = 0; /* Any protocol */
        hints.ai_socktype = SOCK_STREAM;

        int ret = getaddrinfo("", NULL, &hints, &res);
        if (ret == 0 && res)
        {
            addr = (sockaddr_in *)res->ai_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
        }
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
#if WIN32
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
        int keepAlive = 1;   // ����keepalive����. ȱʡֵ: 0(�ر�) 
        int keepIdle = idle_time;   // �����idle����û���κ����ݽ���,�����̽��. ȱʡֵ:7200(s) 
        int keepInterval = interval_time;   // ̽��ʱ��̽�����ʱ����Ϊinterval��. ȱʡֵ:75(s) 
        int keepCount = 2;   // ̽�����ԵĴ���. ȫ����ʱ���϶�����ʧЧ..ȱʡֵ:9(��) 
        setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));
        setsockopt(s, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
        setsockopt(s, SOL_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));
        setsockopt(s, SOL_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));
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
        if ((flags = fcntl(fd, F_GETFL)) < 0)
        {
            LOG << "fcntl get fail";
            return false;
        }

        flags |= O_NONBLOCK; //�޸ķ�������־λ
        if (fcntl(fd, F_SETFL, flags) < 0)
        {
            LOG << "fcntl set fail";
            return false;
        }
        return true
#endif
    }

    bool SetNoDelay(int socket_id)
    {
        (void)socket_id;
        return false;
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
        hint.ai_family = AF_UNSPEC;// Ҳ������AF_INET��AF_INET6ָ��ipv4��ipv6
        hint.ai_socktype = sock_type;

        if ((ret = getaddrinfo(hostname_or_ip.c_str(), nullptr, &hint, &answer)) != 0) {
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

        int sockfd = socket(addr_af, sock_type, 0);
        if (sockfd < 0) {
            LOG_NET_ERR_INFO;
            return -1;
        }

        if (connect(sockfd, addr_ptr, addr_len) < 0) {
            LOG_NET_ERR_INFO;
            CloseSocket(sockfd);
            return -1;
        }

        SetNoBlock(sockfd);

#ifndef WIN32 // �źŻ��������
        {
            const int set = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (const void *)&set, sizeof(int));
        }
#endif
        return sockfd;
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
        // �ظ�ʹ�ñ���ַ��socket�ļ����а�
        // ���������ϵͳ���ᱣ��������ֱ�����һ���ò��ͷţ����̽�����ϵͳ��Ҫ�����Ӻ�������½��а�
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
        return fd;
    }


}



// �����˸���Э�飬��ͷ�����ֽڵĳ��ȣ�| len(2) | data |
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

    // ���������غ���
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
        len += 2;// �������������ֽ�
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
    // �������Ż�
    // buf����һ��Ҫ��2^n����Ч������2^n-1.
    // ���������<2^32��_tail����������
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

    // ������
    virtual void SendMsg(const std::string msg)
    {
        SendOneCode(msg.c_str(), msg.length());
    }
    virtual std::string RecvMsg() {
        char buf[1500] = {0};
        
        while (RecvOneCode(buf, sizeof(buf)) >= 0);
        return buf;
    }
};

class TCPConnect :public NetConnect {
public:
    TCPConnect():_socket_id(-1) {}
    TCPConnect(int socket_id):_socket_id(socket_id){}

    TCPConnect& operator=(int socket_id)
    {
        _socket_id = socket_id;
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
        // UDP���÷ְ�
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

    int GetSocketId()
    {
        return _socket_id;
    }

    int Listen(uint16_t port, std::string bind_address = "")
    {
        _socket_id = NetUtils::Listen(port, SOCK_DGRAM, bind_address);
        return _socket_id;
    }

    void ReflectALL()
    {
        sockaddr_storage client;
        int client_len = sizeof(client);
        char buf[1024];
        int recv_len;
        for (;;)
        {
            recv_len = recvfrom(_socket_id, buf, sizeof(buf), 0, (sockaddr*)&client, &client_len);
            if (recv_len >= 0)
            {
                sendto(_socket_id, buf, recv_len, 0, (sockaddr*)&client, client_len);
            }
            else
            {
                // todo(check errno)
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

    auto local_ip = NetUtils::GetLocalIPAddress();

    LOG << "TCP no block";
    {
        TCPListenConnect listen_connect;
        listen_connect.Listen(8080);
        LOG << "listen " << NetUtils::GetLocalIP(listen_connect.GetSocketId())
            << ":" << NetUtils::GetLocalPort(listen_connect.GetSocketId());
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

    LOG << "UDP no block";
    {
        UDPListenConnect listen_connect;
        listen_connect.Listen(8080);
        LOG << "listen " << NetUtils::GetLocalIP(listen_connect.GetSocketId())
            << ":" << NetUtils::GetLocalPort(listen_connect.GetSocketId());
        UDPConnect client_connect;
        client_connect.Connect(local_ip, 8080);

        client_connect.LogInfo("client");

        client_connect.SendMsg("hello");
        listen_connect.ReflectALL();
        LOG << "client reflect: " << client_connect.RecvMsg();
    }




#if SOCK_TYPE==SOCK_STREAM
    //auto listen_id = ListenOnLocalPort(8088);
    //auto local_ip = GetLocalIPAddress();
    //auto client_id = Connect(local_ip.c_str(), 8088);
    //LOG << "client port:" << GetLocalPort(client_id);
    //auto server_id = Accept(listen_id);
    //LOG << "server port:" << GetLocalPort(server_id);

    //LOG << Write(client_id, "hello");
    //LOG << Write(client_id, "");
    //LOG << LastErrorNo() << LastErrorString();// send���Է�������û�д���
    //LOG << Write(client_id, "world");
    //LOG << "server read:" << Read(server_id);


    //SetKeepAlive(server_id, 10, 1);
    //LOG << "server read:" << Read(server_id);// ��ֹͣ��������ȴ�����

    //CloseSocket(listen_id);
    //CloseSocket(client_id);
    //CloseSocket(server_id);
#endif

#if SOCK_TYPE==SOCK_DGRAM

    auto server_id = ListenOnLocalPort(8080);
    LOG << "server port:" << GetLocalPort(server_id);
    auto local_ip = GetLocalIPAddress();
    auto client_id = Connect(local_ip.c_str(), 8080);
    LOG << "client port:" << GetLocalPort(client_id);

    //shutdown(client_id, SD_BOTH);
    LOG << Write(client_id, "hello");
    LOG << Write(client_id, "");
    //LOG << LastErrorNo() << LastErrorString();
    LOG << Write(client_id, "world");

    LOG << "server read:" << Read(server_id);
    LOG << "server read:" << Read(server_id);
    LOG << "server read:" << Read(server_id);
    LOG << "server read:" << Read(server_id); // ��ȴ�

    CloseSocket(client_id);
    CloseSocket(server_id);
#endif
#ifdef WIN32
    WSACleanup();
#endif // WIN32
}