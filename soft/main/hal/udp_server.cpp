#include <hal/udp_server.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>

#include <string.h>

#include <hal/log.h>

UdpServer::UdpServer(string name, int port) :
    Task(name, Task::Priority::LOW, 8192, false)
{
    int broadcast = 1;

    _name = name;
    _port = port;

    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family      = AF_INET;
    destAddr.sin_port        = htons(_port);

    _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (_socket < 0) 
    {
        LOG_ERROR("Unable to create socket: errno %d", errno);
        return;
    }

    LOG_INFO("Socket created");

    int err = ::bind(_socket, (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (err < 0)
    {
        LOG_ERROR("Socket unable to bind: errno %d", errno);
    }

    if (setsockopt(_socket, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast, sizeof(broadcast)) < 0)
    {
        LOG_ERROR("Failed to set broadcast option");
        closesocket(_socket);
    }

    LOG_INFO("Server ready");

    start();
}

UdpServer::~UdpServer()
{
    close(_socket);
}

void UdpServer::register_callback(function<void(string, string&)> callback)
{
    _callback = callback;
}

bool UdpServer::recv(string &msg)
{
    char buffer[1024];
    int  len;

    len = ::recv(_socket, buffer, sizeof(buffer) - 1, 0);
    if (len > 0)
    {
        buffer[len] = '\0';
        string str(buffer);
        msg = str;
        return true;
    }
    else
    {
        return false;
    }
}

bool UdpServer::recvfrom(string &msg, string &address, int &port)
{
    struct sockaddr_in source_addr;
    socklen_t          source_addr_len = sizeof(source_addr);
    char               buffer[1024];
    char             * addr_char;
    int                len;

    len = ::recvfrom(_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&source_addr, &source_addr_len);

    if (len > 0)
    {
        buffer[len] = '\0';
        string str(buffer);
        msg = str;
        addr_char = inet_ntoa(source_addr.sin_addr);
        string addr_str(addr_char);
        address = addr_str;
        port = ntohs(source_addr.sin_port);
        return true;
    }
    else
    {
        return false;
    }
}

bool UdpServer::sendto(string msg, string address, int port)
{
    struct sockaddr_in dest_addr;
    int                len;

    inet_aton(address.c_str(), &dest_addr.sin_addr);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port   = htons(port);

    len = ::sendto(_socket, msg.c_str(), strlen(msg.c_str()), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (len > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool UdpServer::send_broadcast(string msg, int port)
{
    struct sockaddr_in dest_addr;
    int                len;

    dest_addr.sin_family      = AF_INET;
    dest_addr.sin_port        = htons(port);
    dest_addr.sin_addr.s_addr = IPADDR_BROADCAST;

    len = ::sendto(_socket, msg.c_str(), strlen(msg.c_str()), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (len > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void UdpServer::run()
{
    string      request;
    string      response;
    string      ip_src;
    int         port_src;

    while (1)
    {
        LOG_INFO("Waiting for data");

        if (recvfrom(request, ip_src, port_src))
        {
            LOG_INFO("Received from %s:%d : %s", ip_src.c_str(), port_src, request.c_str());
            _callback(request, response);
            LOG_INFO("Send to %s:%d : %s", ip_src.c_str(), port_src, response.c_str());
            sendto(response, ip_src, port_src);
        }
    }
}