#ifndef XX_HPP
#define XX_HPP

#include <cassert>
#include <set>
#include <memory>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/thread.hpp>

//////////////////////////////////////////////////////////////////////////
//临时写了一个类,发送和接收使用了同一个锁,导致发送时无法接收,接收时无法发送数据.
//实际上应该:发送和接收应该同时进行.
//////////////////////////////////////////////////////////////////////////

class MThIo
{
public:
    ~MThIo() { terminate(); }
    void initialize(int threadNum);
    void terminate();
    boost::asio::io_service& ioService() { return m_io; }
private:
    boost::asio::io_service m_io;
    boost::thread_group     m_thgp;
};
using MThIoPtr = std::shared_ptr<MThIo>;
//////////////////////////////////////////////////////////////////////////

class TcpServerNew;
using BoostSocketPtr = std::unique_ptr<boost::asio::ip::tcp::socket>;
class TcpSocketNew;
using TcpSocketNewPtr = std::shared_ptr<TcpSocketNew>;
class TcpSocketNew :public std::enable_shared_from_this<TcpSocketNew>
{
public:
    TcpSocketNew(boost::asio::io_service& io);
    ///@brief  这个构造函数是专为TcpServer设计的,它设计得有问题,用户可以自行构造一个socket传进去.
    TcpSocketNew(BoostSocketPtr& sock, TcpServerNew* server);
public:
    bool isOpen() const { return m_sock->is_open(); }
    bool isWorking() const { return m_isWorking; }
    bool isConnected() const { return m_isConnected; }
    bool isAccepted() const { return (nullptr != m_server); }
    //
    int connect(const std::string& ip, std::uint16_t port, bool asyncConnect = true);
    void close();
    //
    virtual void onConnected(TcpSocketNewPtr tcpSock) {}
    virtual void onDisconnected(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec) {}
    virtual void onReceivedData(TcpSocketNewPtr tcpSock, const char* data, std::uint32_t size) {}
    virtual void onError(TcpSocketNewPtr tcpSock, int value, const std::string& message) {}
    virtual void onError(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec) {}
    
private:
    void doJustCloseBoostSocket();
    void doConnectAsync(const boost::system::error_code& ec);
    void doConnectAsyncHandler(const boost::system::error_code& ec);
    void doRecvAsync();
    void doRecvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
    //
    void doOnConnected(TcpSocketNewPtr tcpSock);
    void doOnDisconnected(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec);
    void doOnReceivedData(TcpSocketNewPtr tcpSock, const char* data, std::uint32_t size);
    void doOnError(TcpSocketNewPtr tcpSock, int value, const std::string& message);
    void doOnError(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec);
private:
    std::recursive_mutex m_mtx;
    TcpServerNew* const m_server;
    bool m_isWorking;
    bool m_isConnected;
    boost::asio::steady_timer m_timer;
    boost::asio::io_service::strand m_strand;
    const int m_RECVSIZE;
    std::string m_recvBuf;
    BoostSocketPtr m_sock;
    boost::asio::ip::tcp::endpoint m_connectToEp;
};

//////////////////////////////////////////////////////////////////////////

class TcpServerNew
{
    friend class TcpSocketNew;
private:
    typedef std::unique_ptr<boost::asio::ip::tcp::acceptor>  BoostAcceptorPtr;
    typedef std::unique_ptr<boost::asio::io_service::strand> BoostStrandPtr;
public:
    TcpServerNew(boost::asio::io_service& io);
    TcpServerNew(int threadNum);
public:
    int start(const std::string& ip, std::uint16_t port);
    void close();
    //
    virtual void onConnected(TcpSocketNewPtr tcpSock) {}
    virtual void onDisconnected(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec) {}
    virtual void onReceivedData(TcpSocketNewPtr tcpSock, const char* data, std::int32_t size) {}
    virtual void onError(TcpSocketNewPtr tcpSock, int value, const std::string& message) {}
    virtual void onError(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec) {}
private:
    void doAcceptAsync();
    void acceptHandler(const boost::system::error_code& ec, boost::asio::ip::tcp::socket* rawSock);
private:
    std::recursive_mutex m_mtx;
    bool m_isWorking;
    MThIoPtr m_mThIo;
    BoostAcceptorPtr m_acceptor;
    BoostStrandPtr   m_strand;
    std::set<TcpSocketNewPtr> m_set;
};

#include "xx_impl.hpp"
#endif//XX_HPP
