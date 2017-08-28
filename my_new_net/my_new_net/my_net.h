#ifndef MY_TCP_SOCKET_H
#define MY_TCP_SOCKET_H
#include <cassert>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/thread.hpp>
#include <set>
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
    boost::thread_group m_thgp;
    boost::asio::io_service m_io;
};
using MThIoPtr = std::shared_ptr<MThIo>;

#if 1 //MThIo
void MThIo::initialize(int threadNum)
{
    for (int i = 0; i < threadNum; ++i)
    {
        boost::system::error_code ec;
        m_thgp.create_thread(boost::bind(&boost::asio::io_service::run, boost::ref(m_io), ec));
        assert(ec.value() == 0);
    }
}

void MThIo::terminate()
{
    m_io.stop();
    m_thgp.join_all();
}
#endif//MThIo


class TcpServerNew;
using BoostSocketPtr = std::unique_ptr<boost::asio::ip::tcp::socket>;
class TcpSocketNew:public std::enable_shared_from_this<TcpSocketNew>
{
public:
    TcpSocketNew(boost::asio::io_service& io);
    ///@brief  这个构造函数是专为TcpServer设计的,它设计得有问题,用户可以自行构造一个socket传进去.
    TcpSocketNew(BoostSocketPtr& sock, TcpServerNew* server);
public:
    bool isOpen() const { return m_sock->is_open(); }
    bool isWorking() const { return m_isWorking; }
    bool isConnected() const { return m_isConnected; }
    bool isAccepted() const { return (nullptr == m_server); }
    virtual void onConnected(TcpSocketNewPtr tcpSock) {}
    virtual void onDisconnected(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec) {}
    virtual void onReceivedData(TcpSocketNewPtr tcpSock, const char* data, std::uint32_t size) {}
    virtual void onError(TcpSocketNewPtr tcpSock, int value, const std::string& message) {}
    virtual void onError(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec) {}
    int connect(const std::string& ip, std::uint16_t port, bool asyncConnect = true);
    void close();
private:
    void doJustCloseBoostSocket();
    void doConnectAsync(const boost::system::error_code& ec);
    void doConnectAsyncHandler(const boost::system::error_code& ec);
    void doRecvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void doRecvAsync();
    void doOnConnected(TcpSocketNewPtr tcpSock);
    void doOnDisconnected(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec);
    void doOnReceivedData(TcpSocketNewPtr tcpSock, const char* data, std::uint32_t size);
    void doOnError(TcpSocketNewPtr tcpSock, int value, const std::string& message);
    void doOnError(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec);
private:
    std::recursive_mutex m_mtx;
    const TcpServerNew* m_server;
    bool m_isWorking;
    bool m_isConnected;
    boost::asio::steady_timer m_timer;
    boost::asio::io_service::strand m_strand;
    const int m_RECVSIZE;
    std::string m_recvBuf;
    BoostSocketPtr m_sock;
    boost::asio::ip::tcp::endpoint m_connectToEp;
};
using TcpSocketNewPtr = std::shared_ptr<TcpSocketNew>;

#if 1 //TcpSocketNew
TcpSocketNew::TcpSocketNew(boost::asio::io_service& io)
    : m_server(nullptr), m_isWorking(false), m_isConnected(false), m_timer(io), m_strand(io), m_RECVSIZE(4096)
{
    m_sock = BoostSocketPtr(new boost::asio::ip::tcp::socket(io));
    m_recvBuf.resize(m_RECVSIZE);
}

TcpSocketNew::TcpSocketNew(BoostSocketPtr& sock, TcpServerNew* server)
    :m_server(server), m_isWorking(true), m_isConnected(true), m_timer(sock->get_io_service()), m_strand(sock->get_io_service()), m_RECVSIZE(4096)
{
    m_sock = std::move(sock);
    m_recvBuf.resize(m_RECVSIZE);
    
    m_strand.post(boost::bind(&TcpSocketNew::doOnConnected, this, shared_from_this()));
    doRecvAsync();
}

void TcpSocketNew::doJustCloseBoostSocket()
{
    boost::system::error_code ec;
    m_sock->close(ec);
    if (ec.value() != 0)
    {
        m_strand.post(boost::bind(&TcpSocketNew::doOnError, this, shared_from_this(), ec));
        assert(false);
    }

    m_sock->shutdown(boost::asio::socket_base::shutdown_both, ec);
    if (ec.value() != 0)
    {
        m_strand.post(boost::bind(&TcpSocketNew::doOnError, this, shared_from_this(), ec));
        assert(false);
    }
}

void TcpSocketNew::close()
{
    std::lock_guard<std::recursive_mutex> lg(m_mtx);

    if (!m_isWorking)
    {
        assert(m_isConnected == false);
        return;
    }

    m_isWorking = false;
    doJustCloseBoostSocket();//如果m_isConnected==true,需要让接收线程修改它的值.

    m_timer.cancel();
    m_connectToEp = boost::asio::ip::tcp::endpoint();
}

int TcpSocketNew::connect(const std::string& ip, std::uint16_t port, bool asyncConnect = true)
{
    std::lock_guard<std::recursive_mutex> lg(m_mtx);

    if (m_server)
    {
        int value = -1;
        std::string msg = "This is an accepted socket.";
        m_strand.post(boost::bind(&TcpSocketNew::doOnError, this, shared_from_this(), value, msg));
        return value;
    }

    if (m_isWorking)
    {
        int value = -1;//正在工作中的socket不允许connect,请先停止工作.
        std::string msg = "This socket is working.";
        m_strand.post(boost::bind(&TcpSocketNew::doOnError, this, shared_from_this(), value, msg));
        return value;
    }

    assert(m_isConnected == false);

    boost::system::error_code ec;
    boost::asio::ip::address addr = boost::asio::ip::address::from_string(ip, ec);
    if (ec.value() != 0)
    {
        m_strand.post(boost::bind(&TcpSocketNew::doOnError, this, shared_from_this(), ec));
        return ec.value();
    }

    m_connectToEp = boost::asio::ip::tcp::endpoint(addr, port);

    if (asyncConnect)
    {
        doConnectAsync(boost::system::error_code());
        return 0;
    }
    else
    {
        m_sock->connect(m_connectToEp, ec);
        if (ec.value() != 0)
        {
            m_strand.post(boost::bind(&TcpSocketNew::doOnError, this, shared_from_this(), ec));
            return ec.value();
        }

        assert(m_isConnected == false);
        m_strand.post(boost::bind(&TcpSocketNew::doOnConnected, this, shared_from_this()));
        m_isConnected = true;

        doRecvAsync();

        return 0;
    }
}

void TcpSocketNew::doConnectAsync(const boost::system::error_code& ec)//准备发起异步连接.
{
    std::lock_guard<std::recursive_mutex> lg(m_mtx);

    if (ec.value() != 0)
    {
        assert(m_isConnected == false);
        assert(m_isWorking == false);

        m_strand.post(boost::bind(&TcpSocketNew::doOnError, this, shared_from_this(), ec));
        return;
    }

    if (!m_isWorking)
        return;

    assert(m_isConnected == false);

    m_sock->async_connect(m_connectToEp, boost::bind(TcpSocketNew::doConnectAsyncHandler, this, boost::asio::placeholders::error));
}

void TcpSocketNew::doConnectAsyncHandler(const boost::system::error_code& ec)
{
    std::lock_guard<std::recursive_mutex> lg(m_mtx);

    assert(m_isConnected == false);

    if (ec.value() == 0)
    {
        m_isConnected = true;
        assert(m_isWorking == true);
    }

    if (m_isConnected)
    {
        m_strand.post(boost::bind(&TcpSocketNew::doOnConnected, this, shared_from_this()));
        doRecvAsync();
    }
    else
    {
        if (m_isWorking)
        {
            m_timer.expires_from_now(std::chrono::milliseconds(5));
            m_timer.async_wait(boost::bind(&TcpSocketNew::doConnectAsync, this, boost::asio::placeholders::error));
        }
        else
        {   //可能正在连接的过程中,stop了,此时(m_isWorking==false).
            return;
        }
    }
}

void TcpSocketNew::doRecvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (ec.value() != 0)
    {
        std::lock_guard<std::recursive_mutex> lg(m_mtx);

        assert(m_isConnected == true);
        doJustCloseBoostSocket();
        m_strand.post(boost::bind(&TcpSocketNew::doOnDisconnected, this, shared_from_this(), ec));
        m_isConnected = false;

        doConnectAsync(boost::system::error_code());
        return;
    }

    if (bytes_transferred)
    {
        onReceivedData(shared_from_this(), m_recvBuf.c_str(), bytes_transferred);
    }

    doRecvAsync();
}

void TcpSocketNew::doRecvAsync()
{
    m_sock->async_receive(boost::asio::buffer(m_recvBuf), boost::bind(&TcpSocketNew::doRecvAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void TcpSocketNew::doOnConnected(TcpSocketNewPtr tcpSock)
{
    if (m_server)
    {
        m_server->m_set.insert(tcpSock);
        m_server->onConnected(tcpSock);
    }
    onConnected(tcpSock);
}

void TcpSocketNew::doOnDisconnected(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec)
{
    if (m_server)
    {
        m_server->m_set.erase(tcpSock);
        m_server->onDisconnected(tcpSock, ec);
    }
    onDisconnected(tcpSock, ec);
}

void TcpSocketNew::doOnReceivedData(TcpSocketNewPtr tcpSock, const char* data, std::uint32_t size)
{
    if (m_server)
        ;//m_server->;
    onReceivedData(tcpSock, data, size);
}

void TcpSocketNew::doOnError(TcpSocketNewPtr tcpSock, int value, const std::string& message)
{
    if (m_server)
        ;//m_server->;
    onError(tcpSock, value, message);
}

void TcpSocketNew::doOnError(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec)
{
    if (m_server)
        m_server->onError(tcpSock, ec);
    onError(tcpSock, ec);
}

#endif//TcpSocketNew

using BoostStrandPtr = std::shared_ptr<boost::asio::io_service::strand>;
class TcpServerNew
{
    friend class TcpSocketNew;
private:
    typedef std::unique_ptr<boost::asio::ip::tcp::acceptor> BoostAcceptorPtr;
public:
    TcpServerNew(boost::asio::io_service& io);
    TcpServerNew(int threadNum);
    int start(const std::string& ip, std::uint16_t port);
    void close();
    void doAcceptAsync();
    void acceptHandler(const boost::system::error_code& ec, BoostSocketPtr& sock);
    virtual void onConnected(TcpSocketNewPtr tcpSock) {}
    virtual void onDisconnected(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec) {}
    virtual void onReceivedData(TcpSocketNewPtr tcpSock, const char* data, std::int32_t size) {}
    virtual void onError(TcpSocketNewPtr tcpSock, int value, const std::string& message) {}
    virtual void onError(TcpSocketNewPtr tcpSock, const boost::system::error_code& ec) {}
private:
    std::recursive_mutex m_mtx;
    bool m_isWorking;
    MThIoPtr m_mThIo;
    BoostAcceptorPtr m_acceptor;
    BoostStrandPtr m_strand;
    std::set<TcpSocketNewPtr> m_set;
};

#if 1 //TcpServerNew
TcpServerNew::TcpServerNew(boost::asio::io_service& io) :m_isWorking(false)
{
    m_mThIo = nullptr;
    m_acceptor = BoostAcceptorPtr(new boost::asio::ip::tcp::acceptor(io));
    m_strand = BoostStrandPtr(new boost::asio::io_service::strand(io));
    m_set.clear();
}

TcpServerNew::TcpServerNew(int threadNum) :m_isWorking(false)
{
    m_mThIo = MThIoPtr(new MThIo);
    m_mThIo->initialize(threadNum);
    m_acceptor = BoostAcceptorPtr(new boost::asio::ip::tcp::acceptor(m_mThIo->ioService()));
    m_strand = BoostStrandPtr(new boost::asio::io_service::strand(m_mThIo->ioService()));
    m_set.clear();
}

int TcpServerNew::start(const std::string& ip, std::uint16_t port)
{
    std::lock_guard<std::recursive_mutex> lg(m_mtx);

    if (m_isWorking)
    {
        int value = -1;
        std::string msg = "TcpServerNew is working.";
        m_strand->post(boost::bind(&TcpServerNew::onError, this, nullptr, value, msg));
        return value;
    }

    boost::system::error_code ec;
    boost::asio::ip::address addr = boost::asio::ip::address::from_string(ip, ec);
    if (ec.value() != 0)
    {
        m_strand->post(boost::bind(&TcpServerNew::onError, this, nullptr, ec));
        return ec.value();
    }

    boost::asio::ip::tcp::endpoint ep(addr, port);
    m_acceptor->bind(ep, ec);
    if (ec.value() != 0)
    {
        m_strand->post(boost::bind(&TcpServerNew::onError, this, nullptr, ec));
        return ec.value();
    }

    const int backlog = 10;
    m_acceptor->listen(backlog, ec);
    if (ec.value() != 0)
    {
        m_strand->post(boost::bind(&TcpServerNew::onError, this, nullptr, ec));
        return ec.value();
    }

    m_isWorking = true;

    doAcceptAsync();
}

void TcpServerNew::close()
{
    std::lock_guard<std::recursive_mutex> lg(m_mtx);

    if (!m_isWorking)
    {
        int value = -1;
        std::string msg = "TcpServerNew not working.";
        m_strand->post(boost::bind(&TcpServerNew::onError, this, nullptr, value, msg));
        return;
    }
    else
    {
        m_isWorking = false;
        boost::system::error_code ec;
        m_acceptor->close(ec);
        if (ec.value() != 0)
        {
            m_strand->post(boost::bind(&TcpServerNew::onError, this, nullptr, ec));
            assert(false);
        }
    }
}

void TcpServerNew::doAcceptAsync()
{
    BoostSocketPtr sock = BoostSocketPtr(new boost::asio::ip::tcp::socket(m_acceptor->get_io_service()));
    m_acceptor->async_accept(*sock, boost::bind(&TcpServerNew::acceptHandler, this, boost::asio::placeholders::error, std::move(sock)));
}

void TcpServerNew::acceptHandler(const boost::system::error_code& ec, BoostSocketPtr& sock)
{
    if (ec.value() != 0)
    {
        std::lock_guard<std::recursive_mutex> lg(m_mtx);
        m_strand->post(boost::bind(&TcpServerNew::onError, this, nullptr, ec));
        assert(m_isWorking == false);
        return;
    }

    TcpSocketNewPtr tcpSock = TcpSocketNewPtr(new TcpSocketNew(sock, this));
    //TODO:写得不好,TcpSocketNew的构造函数里,将数据存储进了set.
    doAcceptAsync();
}
#endif//TcpServerNew

#endif//MY_TCP_SOCKET_H
