#ifndef XX_HPP
#define XX_HPP


#include <cassert>
#include <string>
#include <set>
#include <memory>
#include <mutex>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/thread.hpp>


//////////////////////////////////////////////////////////////////////////
//临时写了一个类,发送和接收使用了同一个锁,导致发送时无法接收,接收时无法发送数据.
//实际上应该:发送和接收应该同时进行.
//////////////////////////////////////////////////////////////////////////


class IoWithMTh
{
public:
    IoWithMTh() :m_wk(m_io) {}
    virtual ~IoWithMTh() { terminate(); }
    boost::asio::io_service& ioService() { return m_io; }
    void initialize(int threadNum);
    void terminate();
private:
    boost::asio::io_service m_io;
    boost::asio::io_service::work m_wk;
    boost::thread_group m_thgp;
};
using IoWithMThPtr = std::shared_ptr<IoWithMTh>;


//////////////////////////////////////////////////////////////////////////


class TcpSocket;
using TcpSocketPtr = std::shared_ptr<TcpSocket>;


class TcpServer
{
private:
    friend class TcpSocket;
private:
    typedef std::unique_ptr<boost::asio::ip::tcp::acceptor>  BoostAcceptorPtr;
    typedef std::unique_ptr<boost::asio::io_service::strand> BoostStrandPtr;

public:
    TcpServer(boost::asio::io_service& io);
    TcpServer(int threadNum);

public:
    void open(const std::string& ip, std::uint16_t port, int&eValue, std::string& eMessage);
    int  open(const std::string& ip, std::uint16_t port);
    void close(int&eValue, std::string& eMessage);
    int  close();
    //
    virtual void onConnected(TcpSocketPtr tcpSock)/* {}*/;
    virtual void onDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec)/* {}*/;
    virtual void onReceivedData(TcpSocketPtr tcpSock, const char* data, std::uint32_t size)/* {}*/;
    virtual void onError(TcpSocketPtr tcpSock, const boost::system::error_code& ec)/* {}*/;
    virtual void onError(TcpSocketPtr tcpSock, int value, const std::string& message)/* {}*/;

private:
    void doAcceptAsync();
    void doAcceptAsyncHandler(const boost::system::error_code& ec, boost::asio::ip::tcp::socket* rawSock);

private:
    std::recursive_mutex m_mtx;
    bool m_isWorking;
    IoWithMThPtr m_mThIo;
    BoostAcceptorPtr m_acceptor;
    boost::asio::ip::tcp::endpoint m_localEndpoint;
    BoostStrandPtr   m_strand;
    std::set<TcpSocketPtr> m_set;
};


//////////////////////////////////////////////////////////////////////////


using BoostSocketPtr = std::unique_ptr<boost::asio::ip::tcp::socket>;


class TcpSocket :public std::enable_shared_from_this<TcpSocket>
{
private:
    class SendBuffer
    {
    public:
        SendBuffer() :m_capacity(1024 * 512), m_isSending(false) { reset(); }
        void reset();
    public:
        std::size_t const m_capacity;
        std::string m_bufWait;
        std::string m_bufWork;
        std::size_t m_posWork;//当前的位置,再往前的数据都被发送过了,都是过时的数据了.
        bool        m_isSending;
    };
    //
    class RecvBuffer
    {
    public:
        RecvBuffer() :m_BUFSIZE(1024 * 8) { m_bufWork.resize(m_BUFSIZE); }
    public:
        std::string m_bufWork;
        std::size_t m_BUFSIZE;
    };

public:
    TcpSocket(boost::asio::io_service& io);
    TcpSocket(BoostSocketPtr& sock, TcpServer* server);//TODO:

public:
    bool isAccepted() const { return (nullptr != m_server); }
    bool isWorking() const { return m_isWorking; }
    bool isConnected() const { return m_isConnected; }
    bool isOpen() const { return m_sock->is_open(); }
    const boost::asio::ip::tcp::endpoint& bindEndpoint() { return m_server ? m_server->m_localEndpoint : m_defaultEndpoint; }
    const boost::asio::ip::tcp::endpoint& connectEndpoint() { return m_peerEndpoint; }
    const boost::asio::ip::tcp::endpoint& localEndpoint() { return m_localEndpoint; }
    const boost::asio::ip::tcp::endpoint& remoteEndpoint() { return m_remoteEndpoint; }
    const boost::asio::ip::tcp::endpoint& localEndpointEx() { return m_server ? bindEndpoint() : localEndpoint(); }
    const boost::asio::ip::tcp::endpoint& remoteEndpointEx() { return m_server ? remoteEndpoint() : connectEndpoint(); }
    //
    void connect(const std::string& ip, std::uint16_t port, bool asyncConnect, int& eValue, std::string& eMessage);
    int  connect(const std::string& ip, std::uint16_t port, bool asyncConnect = true);
    void close();
    void sendAsync(const char* data, std::uint32_t size, int&eValue, std::string& eMeseage);
    int  sendAsync(const char* data, std::uint32_t size);
    int  sendSynch(const char* data, std::uint32_t size, int&eValue, std::string& eMeseage);
    //
    virtual void onConnected(TcpSocketPtr tcpSock)/* {}*/;
    virtual void onDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec)/* {}*/;
    virtual void onReceivedData(TcpSocketPtr tcpSock, const char* data, std::uint32_t size)/* {}*/;
    virtual void onError(TcpSocketPtr tcpSock, int value, const std::string& message)/* {}*/;
    virtual void onError(TcpSocketPtr tcpSock, const boost::system::error_code& ec)/* {}*/;

private:
    void doJustCloseBoostSocket();
    void doConnectAsync(const boost::system::error_code& ec);
    void doConnectAsyncHandler(const boost::system::error_code& ec);
    void doRecvAsync();
    void doRecvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void doSendAsync(std::size_t lastLengthSent);
    void doSendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
    //
    void doOnConnected(TcpSocketPtr tcpSock);
    void doOnDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec);
    void doOnReceivedData(TcpSocketPtr tcpSock, const char* data, std::uint32_t size);
    void doOnError(TcpSocketPtr tcpSock, const boost::system::error_code& ec);
    void doOnError(TcpSocketPtr tcpSock, int value, const std::string& message);
private:
    std::recursive_mutex            m_mtx;
    boost::asio::ip::tcp::endpoint  m_peerEndpoint;
    TcpServer* const                m_server;
    bool                            m_isWorking;
    bool                            m_isConnected;
    boost::asio::steady_timer       m_timer;
    boost::asio::io_service::strand m_strand;
    BoostSocketPtr                  m_sock;
    SendBuffer                      m_sendBuf;
    RecvBuffer                      m_recvBuf;

    boost::asio::ip::tcp::endpoint m_localEndpoint;
    boost::asio::ip::tcp::endpoint m_remoteEndpoint;
    boost::asio::ip::tcp::endpoint m_defaultEndpoint;
};


#include "xx_impl.hpp"
#endif//XX_HPP
