#ifndef xx_impl_H
#define xx_impl_H
#include "xx.hpp"

namespace MyNet
{
#define TcpSocket_is_already_working   "TcpSocket is already working."
#define TcpSocket_is_no_longer_working "TcpSocket is no longer working."
#define TcpSocket_has_been_connected   "TcpSocket has been connected."
#define TcpSocket_is_not_connected     "TcpSocket is not connected."
#define TcpSocket_is_already_receiving "TcpSocket is already receiving."
#define TcpSocket_is_being_sent_async  "TcpSocket is being sent async."
#define This_is_an_accepted_socket     "This is an accepted socket."
#define TcpServer_is_already_working   "TcpServer is already working."
#define TcpServer_is_no_longer_working "TcpServer is no longer working."
#define peer_endpoint_is_unreachable   "peer_endpoint is unreachable."
};

namespace MyNet
{
    void IoWithMTh::initialize(int threadNum)//这个类只能使用一次.
    {
        if (m_thgp.size() != 0)
            return;

        for (int i = 0; i < threadNum; ++i)
        {
            boost::system::error_code ec;
            m_thgp.create_thread(boost::bind(&boost::asio::io_service::run, boost::ref(m_io), ec));
            assert(ec.value() == 0);
        }
    }

    void IoWithMTh::terminate()
    {
        m_io.stop();
        m_thgp.join_all();
    }
}


namespace MyNet
{
    void TcpSocket::SendBuffer::reset()
    {
        m_bufWait.clear();
        m_bufWork.clear();
        m_posWork = 0;

        if (m_bufWait.capacity() < m_capacity)
            m_bufWait.reserve(m_capacity);

        if (m_bufWork.capacity() < m_capacity)
            m_bufWork.reserve(m_capacity);

        //m_isSending = false;//该字段应该是由socket类修改.
    }

    TcpSocket::TcpSocket(boost::asio::io_service& netIo)
        :m_server(nullptr), m_isWorking(false), m_isConnected(false),
        m_timer(netIo)
    {
        m_netStrand = BoostStrandPtr(new boost::asio::io_service::strand(netIo));
        m_appStrand = m_netStrand;
        m_socket = BoostSocketPtr(new boost::asio::ip::tcp::socket(netIo));
    }

    TcpSocket::TcpSocket(boost::asio::io_service& netIo, boost::asio::io_service& appIo)
        :m_server(nullptr), m_isWorking(false), m_isConnected(false),
        m_timer(netIo)
    {
        m_netStrand = BoostStrandPtr(new boost::asio::io_service::strand(netIo));
        m_appStrand = BoostStrandPtr(new boost::asio::io_service::strand(appIo));
        m_socket = BoostSocketPtr(new boost::asio::ip::tcp::socket(netIo));
    }

    ///@brief  这个构造函数是专为TcpServer设计的,它设计得有问题,用户可以自行构造一个socket传进去.
    TcpSocket::TcpSocket(BoostSocketPtr& sock, TcpServer* server)
        :m_server(server), m_isWorking(true), m_isConnected(true),
        m_timer(sock->get_io_service())
    {
        assert(nullptr != m_server);

        m_netStrand = BoostStrandPtr(new boost::asio::io_service::strand(sock->get_io_service()));
        if (server->m_netStrand.get() == server->m_appStrand.get())
        {
            m_appStrand = m_netStrand;
        }
        else
        {
            m_appStrand = BoostStrandPtr(new boost::asio::io_service::strand(server->m_appStrand->get_io_service()));
        }
        m_socket = std::move(sock);
        m_localEndpoint = m_socket->local_endpoint();
        m_remoteEndpoint = m_socket->remote_endpoint();

        m_netStrand->post(boost::bind(&TcpSocket::doOnConnected, this, shared_from_this()));
        doRecvAsync();
    }

    int TcpSocket::connect(const std::string& ip, std::uint16_t port, bool asyncConnect, std::string& eMessage)
    {
        int eValue = 0;
        std::lock_guard<std::recursive_mutex> lg(m_mtx);

        if (m_server)
        {
            eValue = -1;
            eMessage = This_is_an_accepted_socket;
            return eValue;
        }

        if (m_isWorking)
        {
            eValue = -1;
            eMessage = TcpSocket_is_already_working;
            return eValue;
        }

        assert(m_isConnected == false);

        boost::system::error_code ec;
        boost::asio::ip::address addr = boost::asio::ip::address::from_string(ip, ec);
        if (ec.value() != 0)
        {
            eValue = ec.value();
            eMessage = ec.message();
            return eValue;
        }

        m_peerEndpoint = boost::asio::ip::tcp::endpoint(addr, port);

        if (asyncConnect)
        {
            m_isWorking = true;
            doConnectAsync(boost::system::error_code());
            return eValue;
        }
        else
        {
            m_socket->connect(m_peerEndpoint, ec);

            if (ec.value() != 0)
            {
                eValue = ec.value();
                eMessage = ec.message();
                return eValue;
            }
            else
            {
                assert(false == m_isWorking);
                m_isWorking = true;
                assert(false == m_isConnected);
                m_isConnected = true;

                m_localEndpoint = m_socket->local_endpoint();
                m_remoteEndpoint = m_socket->remote_endpoint();

                m_netStrand->post(boost::bind(&TcpSocket::doOnConnected, this, shared_from_this()));

                doRecvAsync();
                return eValue;
            }
        }
    }

    int TcpSocket::connect(const std::string& ip, std::uint16_t port, bool asyncConnect /* = true */)
    {
        std::string eMessage;
        int eValue = connect(ip, port, asyncConnect, eMessage);

        if (eValue)
        {
            m_netStrand->post(boost::bind(&TcpSocket::onError, this, shared_from_this(), eValue, eMessage));
        }

        return eValue;
    }

    void TcpSocket::close()
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
        m_peerEndpoint = boost::asio::ip::tcp::endpoint();
    }

    int TcpSocket::sendAsync(const char* data, std::uint32_t size, std::string& eMeseage)
    {
        int eValue = 0;
        std::lock_guard<std::recursive_mutex> lg(m_mtx);

        if (!m_isWorking)
        {
            eValue = -1;
            eMeseage = TcpSocket_is_no_longer_working;
            return eValue;
        }

        if (!m_isConnected)
        {
            eValue = -1;
            eMeseage = TcpSocket_is_not_connected;
            return eValue;
        }

        m_sendBuf.m_bufWait.append(data, size);

        if (!m_sendBuf.m_isSending)
        {
            doSendAsync(0);
        }

        return eValue;
    }

    int TcpSocket::sendAsync(const char* data, std::uint32_t size)
    {
        std::string eMessage;
        int eValue = sendAsync(data, size, eMessage);

        if (eValue)
        {
            m_netStrand->post(boost::bind(&TcpSocket::onError, this, shared_from_this(), eValue, eMessage));
        }

        return eValue;
    }

    int TcpSocket::sendSynch(const char* data, std::uint32_t size, int&bytesTransferred, std::string& eMeseage)
    {
        int eValue = 0;
        bytesTransferred = 0;
        std::lock_guard<std::recursive_mutex> lg(m_mtx);

        if (!m_isWorking)
        {
            eValue = -1;
            eMeseage = TcpSocket_is_no_longer_working;
            return eValue;
        }

        if (!m_isConnected)
        {
            eValue = -1;
            eMeseage = TcpSocket_is_not_connected;
            return eValue;
        }

        if (m_sendBuf.m_isSending)
        {
            eValue = -1;
            eMeseage = TcpSocket_is_being_sent_async;
            return eValue;
        }

        boost::system::error_code ec;
        bytesTransferred = m_socket->send(boost::asio::buffer(data, size), 0, ec);

        eValue = ec.value();
        eMeseage = ec.value() ? ec.message() : "";

        return eValue;
    }

    void TcpSocket::doJustCloseBoostSocket()
    {
        boost::system::error_code ec;
        m_socket->shutdown(boost::asio::socket_base::shutdown_both, ec);
        m_socket->close(ec);
    }

    void TcpSocket::doConnectAsync(const boost::system::error_code& ec)//准备发起异步连接.
    {
        std::lock_guard<std::recursive_mutex> lg(m_mtx);

        if (ec.value() != 0)
        {
            assert(m_isConnected == false);
            assert(m_isWorking == false);

            m_netStrand->post(boost::bind(&TcpSocket::doOnError, this, shared_from_this(), ec));
            return;
        }

        if (!m_isWorking)
            return;

        assert(m_isConnected == false);

        m_socket->async_connect(m_peerEndpoint, boost::bind(&TcpSocket::doConnectAsyncHandler, this, boost::asio::placeholders::error));
    }

    void TcpSocket::doConnectAsyncHandler(const boost::system::error_code& ec)
    {
        std::lock_guard<std::recursive_mutex> lg(m_mtx);

        assert(m_isConnected == false);

        if (ec.value() == 0)
        {
            assert(m_isWorking == true);
            m_isConnected = true;

            m_localEndpoint = m_socket->local_endpoint();
            m_remoteEndpoint = m_socket->remote_endpoint();
        }

        if (m_isConnected)
        {
            m_netStrand->post(boost::bind(&TcpSocket::doOnConnected, this, shared_from_this()));
            doRecvAsync();
        }
        else
        {
            if (m_isWorking)
            {
                m_timer.expires_from_now(std::chrono::milliseconds(5));
                m_timer.async_wait(boost::bind(&TcpSocket::doConnectAsync, this, boost::asio::placeholders::error));
            }
            else
            {   //可能正在连接的过程中,stop了,此时(m_isWorking==false).
                return;
            }
        }
    }

    void TcpSocket::doRecvAsync()
    {
        const static int RECV_BUF_SIZE = 1792;
        m_recvBuf = StdStringPtr(new std::string());
        m_recvBuf->resize(RECV_BUF_SIZE);
        m_socket->async_receive(
            boost::asio::buffer(const_cast<char*>(m_recvBuf->c_str()), m_recvBuf->size()),
            boost::bind(&TcpSocket::doRecvAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
        );//TODO:
    }

    void TcpSocket::doRecvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec.value() != 0)
        {
            std::lock_guard<std::recursive_mutex> lg(m_mtx);

            if (m_isConnected)//可能异步send和异步recv同时接到了错误信息.
            {
                doJustCloseBoostSocket();

                m_localEndpoint = boost::asio::ip::tcp::endpoint();
                m_remoteEndpoint = boost::asio::ip::tcp::endpoint();

                m_isConnected = false;

                m_netStrand->post(boost::bind(&TcpSocket::doOnDisconnected, this, shared_from_this(), ec));
            }

            doConnectAsync(boost::system::error_code());
            return;
        }

        if (0 < bytes_transferred)
        {
            m_recvBuf->resize(bytes_transferred);
            m_appStrand->post(boost::bind(&TcpSocket::onReceivedData, this, shared_from_this(), m_recvBuf));
            m_recvBuf = nullptr;
        }

        doRecvAsync();
    }

    void TcpSocket::doSendAsync(std::size_t lastLengthSent)
    {
        std::lock_guard<std::recursive_mutex> lg(m_mtx);

        const char* bufData = nullptr;
        std::size_t bufSize = 0;

        m_sendBuf.m_posWork += lastLengthSent;
        assert(m_sendBuf.m_posWork <= m_sendBuf.m_bufWork.size());

        if (m_sendBuf.m_bufWork.size() == m_sendBuf.m_posWork)
        {
            m_sendBuf.m_bufWork.clear();
            m_sendBuf.m_posWork = 0;
            std::swap(m_sendBuf.m_bufWork, m_sendBuf.m_bufWait);
        }

        bufSize = m_sendBuf.m_bufWork.size() - m_sendBuf.m_posWork;
        if (bufSize)
        {
            bufData = m_sendBuf.m_bufWork.c_str() + m_sendBuf.m_posWork;
            m_sendBuf.m_isSending = true;
        }
        else
        {
            m_sendBuf.m_isSending = false;
            return;
        }

        //我没必要写代码判断socket的状态,让asio代我检查即可.
        m_socket->async_send(boost::asio::buffer(bufData, bufSize), /*用std::bind会出错*/boost::bind(&TcpSocket::doSendAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void TcpSocket::doSendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec.value() != 0)
        {
            std::lock_guard<std::recursive_mutex> lg(m_mtx);

            if (m_isConnected)//可能异步send和异步recv同时接到了错误信息.
            {
                doJustCloseBoostSocket();

                m_localEndpoint = boost::asio::ip::tcp::endpoint();
                m_remoteEndpoint = boost::asio::ip::tcp::endpoint();

                m_isConnected = false;

                m_netStrand->post(boost::bind(&TcpSocket::doOnDisconnected, this, shared_from_this(), ec));
            }

            m_sendBuf.reset();
            m_sendBuf.m_isSending = false;

            doConnectAsync(boost::system::error_code());
        }
        else
        {
            doSendAsync(bytes_transferred);
        }
    }

    void TcpSocket::doOnConnected(TcpSocketPtr tcpSock)
    {
        if (m_server)
        {
            m_server->addSocket(tcpSock);
            m_server->onConnected(tcpSock);
        }
        onConnected(tcpSock);
    }

    void TcpSocket::doOnDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec)
    {
        if (m_server)
        {
            m_server->delSocket(tcpSock);
            m_server->onDisconnected(tcpSock, ec);
        }
        onDisconnected(tcpSock, ec);
    }

    void TcpSocket::doOnReceivedData(TcpSocketPtr tcpSock, StdStringPtr data)
    {
        if (m_server)
        {
            m_server->onReceivedData(tcpSock, data);
        }
        onReceivedData(tcpSock, data);
    }

    void TcpSocket::doOnError(TcpSocketPtr tcpSock, const boost::system::error_code& ec)
    {
        if (m_server)
        {
            m_server->onError(tcpSock, ec);
        }
        onError(tcpSock, ec);
    }

    void TcpSocket::doOnError(TcpSocketPtr tcpSock, int value, const std::string& message)
    {
        if (m_server)
        {
            m_server->onError(tcpSock, value, message);
        }
        onError(tcpSock, value, message);
    }
};


namespace MyNet
{
    TcpServer::TcpServer(boost::asio::io_service& netIo)
        :m_isWorking(false)
    {
        m_netMThIo = nullptr;
        m_appMThIo = nullptr;
        m_netStrand = BoostStrandPtr(new boost::asio::io_service::strand(netIo));
        m_appStrand = m_netStrand;
        m_acceptor = BoostAcceptorPtr(new boost::asio::ip::tcp::acceptor(netIo));
        m_localEndpoint;
        m_set.clear();
    }

    TcpServer::TcpServer(boost::asio::io_service& netIo, boost::asio::io_service& appIo)
        :m_isWorking(false)
    {
        m_netMThIo = nullptr;
        m_appMThIo = nullptr;
        m_netStrand = BoostStrandPtr(new boost::asio::io_service::strand(netIo));
        m_appStrand = BoostStrandPtr(new boost::asio::io_service::strand(appIo));
        m_acceptor = BoostAcceptorPtr(new boost::asio::ip::tcp::acceptor(netIo));
        m_localEndpoint;
        m_set.clear();
    }

    TcpServer::TcpServer(int netThNum, int appThNum)
        :m_isWorking(false)
    {
        assert(0 < netThNum);

        m_netMThIo = IoWithMThPtr(new IoWithMTh);
        m_netMThIo->initialize(netThNum);
        m_netStrand = BoostStrandPtr(new boost::asio::io_service::strand(m_netMThIo->ioService()));
        if (0 < appThNum)
        {
            m_appMThIo = IoWithMThPtr(new IoWithMTh);
            m_appMThIo->initialize(appThNum);
            m_appStrand = BoostStrandPtr(new boost::asio::io_service::strand(m_appMThIo->ioService()));
        }
        else
        {
            m_appStrand = m_netStrand;
        }
        m_acceptor = BoostAcceptorPtr(new boost::asio::ip::tcp::acceptor(m_netMThIo->ioService()));
        m_localEndpoint;
        m_set.clear();
    }

    int TcpServer::open(const std::string& ip, std::uint16_t port, std::string& eMessage)
    {
        int eValue = 0;
        std::lock_guard<std::recursive_mutex> lg(m_mtx);

        if (m_isWorking)
        {
            eValue = -1;
            eMessage = TcpServer_is_already_working;
            return eValue;
        }

        boost::system::error_code ec;
        boost::asio::ip::address addr = boost::asio::ip::address::from_string(ip, ec);
        if (ec.value() != 0)
        {
            eValue = ec.value();
            eMessage = ec.message();
            return eValue;
        }

        m_localEndpoint = boost::asio::ip::tcp::endpoint(addr, port);

        m_acceptor->bind(m_localEndpoint, ec);
        if (ec.value() != 0)
        {
            eValue = ec.value();
            eMessage = ec.message();
            return eValue;
        }

        const int backlog = 10;
        m_acceptor->listen(backlog, ec);
        if (ec.value() != 0)
        {
            eValue = ec.value();
            eMessage = ec.message();
            assert(false);
            return eValue;
        }

        m_isWorking = true;

        doAcceptAsync();

        return eValue;
    }

    int TcpServer::open(const std::string& ip, std::uint16_t port)
    {
        std::string eMessage;
        int eValue = open(ip, port, eMessage);

        if (eValue)
        {
            m_netStrand->post(boost::bind(&TcpServer::onError, this, nullptr, eValue, eMessage));
        }

        return eValue;
    }

    int TcpServer::close(std::string& eMessage)
    {
        int eValue = 0;
        std::lock_guard<std::recursive_mutex> lg(m_mtx);

        if (!m_isWorking)
        {
            eValue = -1;
            eMessage = TcpServer_is_no_longer_working;
        }
        else
        {
            m_isWorking = false;

            boost::system::error_code ec;
            m_acceptor->close(ec);

            if (ec.value() != 0)
            {
                eValue = ec.value();
                eMessage = ec.message();
            }

            m_localEndpoint = boost::asio::ip::tcp::endpoint();

            for (TcpSocketPtr sock : m_set)
            {
                sock->close();
            }
            m_set.clear();
        }

        return eValue;
    }

    int TcpServer::close()
    {
        std::string eMessage;
        int eValue = close(eMessage);

        if (eValue)
        {
            m_netStrand->post(boost::bind(&TcpServer::onError, this, nullptr, eValue, eMessage));
        }

        return eValue;
    }

    void TcpServer::doAcceptAsync()
    {
        boost::asio::ip::tcp::socket* rawSock = new boost::asio::ip::tcp::socket(m_acceptor->get_io_service());
        m_acceptor->async_accept(*rawSock, boost::bind(&TcpServer::doAcceptAsyncHandler, this, boost::asio::placeholders::error, rawSock));
    }

    void TcpServer::doAcceptAsyncHandler(const boost::system::error_code& ec, boost::asio::ip::tcp::socket* rawSock)
    {
        assert(rawSock != nullptr);
        BoostSocketPtr sock = BoostSocketPtr(rawSock);

        if (ec.value() != 0)
        {
            std::lock_guard<std::recursive_mutex> lg(m_mtx);
            m_netStrand->post(boost::bind(&TcpServer::onError, this, nullptr, ec));
            assert(m_isWorking == false);
            return;
        }

        TcpSocketPtr tcpSock = TcpSocketPtr(new TcpSocket(sock, this));
        //TODO:写得不好,TcpSocketNew的构造函数里,将数据存储进了set.
        doAcceptAsync();
    }

    void TcpServer::addSocket(TcpSocketPtr sock)
    {
        std::lock_guard<std::recursive_mutex> lg(m_mtx);
        m_set.insert(sock);
    }

    void TcpServer::delSocket(TcpSocketPtr sock)
    {
        std::lock_guard<std::recursive_mutex> lg(m_mtx);
        m_set.erase(sock);
    }
};


namespace MyNet
{
    std::string endPoint2string(const boost::asio::ip::tcp::endpoint& ep)
    {
        char buf[128] = { 0 };
        std::snprintf(buf, sizeof(buf), "%s:%d", ep.address().to_string().c_str(), ep.port());
        return buf;
    }
    //////////////////////////////////////////////////////////////////////////
    void TcpSocket::onConnected(TcpSocketPtr tcpSock)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpSocket,onConnected___,socket=[%p],localEx=[%s],remoteEx=[%s]", tcpSock.get(),
            endPoint2string(tcpSock->localEndpointEx()).c_str(), endPoint2string(tcpSock->remoteEndpointEx()).c_str());
        std::cout << msgBuf << std::endl;
    }
    void TcpSocket::onDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpSocket,onDisconnected,socket=[%p],value=[%d],message=[%s]", tcpSock.get(),
            ec.value(), ec.message().c_str());
        std::cout << msgBuf << std::endl;
    }
    void TcpSocket::onError(TcpSocketPtr tcpSock, const boost::system::error_code& ec)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpSocket,onErrorMessage,socket=[%p],value=[%d],message=[%s]", tcpSock.get(),
            ec.value(), ec.message().c_str());
        std::cout << msgBuf << std::endl;
    }
    void TcpSocket::onError(TcpSocketPtr tcpSock, int value, const std::string& message)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpSocket,onErrorMessage,socket=[%p],value=[%d],message=[%s]", tcpSock.get(),
            value, message.c_str());
        std::cout << msgBuf << std::endl;
    }
    void TcpSocket::onReceivedData(TcpSocketPtr tcpSock, StdStringPtr data)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpSocket,onReceivedData,socket=[%p],size=[%d],data=[%s]", tcpSock.get(),
            int(data->size()), data->c_str());
        std::cout << msgBuf << std::endl;
    }
    //////////////////////////////////////////////////////////////////////////
    void TcpServer::onConnected(TcpSocketPtr tcpSock)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpServer,onConnected___,socket=[%p],localEx=[%s],remoteEx=[%s]", tcpSock.get(),
            endPoint2string(tcpSock->localEndpointEx()).c_str(), endPoint2string(tcpSock->remoteEndpointEx()).c_str());
        std::cout << msgBuf << std::endl;
    }
    void TcpServer::onDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpServer,onDisconnected,socket=[%p],value=[%d],message=[%s]", tcpSock.get(),
            ec.value(), ec.message().c_str());
        std::cout << msgBuf << std::endl;
    }
    void TcpServer::onError(TcpSocketPtr tcpSock, const boost::system::error_code& ec)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpServer,onErrorMessage,socket=[%p],value=[%d],message=[%s]", tcpSock.get(),
            ec.value(), ec.message().c_str());
        std::cout << msgBuf << std::endl;
    }
    void TcpServer::onError(TcpSocketPtr tcpSock, int value, const std::string& message)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpServer,onErrorMessage,socket=[%p],value=[%d],message=[%s]", tcpSock.get(),
            value, message.c_str());
        std::cout << msgBuf << std::endl;
    }
    void TcpServer::onReceivedData(TcpSocketPtr tcpSock, StdStringPtr data)
    {
        char msgBuf[1024] = { 0 };
        std::snprintf(msgBuf, sizeof(msgBuf) - 1, "TcpServer,onReceivedData,socket=[%p],size=[%d],data=[%s]", tcpSock.get(),
            int(data->size()), data->c_str());
        std::cout << msgBuf << std::endl;
    }
};


namespace MyNet
{
    void TcpSocketTemp::onConnected(TcpSocketPtr tcpSock)
    {
        m_client->onConnected(tcpSock);
    }
    void TcpSocketTemp::onDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec)
    {
        m_client->onDisconnected(tcpSock, ec);
    }
    void TcpSocketTemp::onReceivedData(TcpSocketPtr tcpSock, StdStringPtr data)
    {
        m_client->onReceivedData(tcpSock, data);
    }
    void TcpSocketTemp::onError(TcpSocketPtr tcpSock, int value, const std::string& message)
    {
        m_client->onError(tcpSock, value, message);
    }
    void TcpSocketTemp::onError(TcpSocketPtr tcpSock, const boost::system::error_code& ec)
    {
        m_client->onError(tcpSock, ec);
    }

    //////////////////////////////////////////////////////////////////////////

    TcpClient::TcpClient(boost::asio::io_service& netIo)
    {
        m_netMThIo = nullptr;
        m_appMThIo = nullptr;
        m_sock = std::shared_ptr<TcpSocketTemp>(new TcpSocketTemp(netIo));
        m_sock->setPointer(this);
    }

    TcpClient::TcpClient(boost::asio::io_service& netIo, boost::asio::io_service& appIo)
    {
        m_netMThIo = nullptr;
        m_appMThIo = nullptr;
        m_sock = std::shared_ptr<TcpSocketTemp>(new TcpSocketTemp(netIo, appIo));
        m_sock->setPointer(this);
    }

    TcpClient::TcpClient(int netThNum, int appThNum)
    {
        m_netMThIo = IoWithMThPtr(new IoWithMTh);
        m_netMThIo->initialize(netThNum);
        m_appMThIo = IoWithMThPtr(new IoWithMTh);
        m_appMThIo->initialize(appThNum);
        m_sock = std::shared_ptr<TcpSocketTemp>(new TcpSocketTemp(m_netMThIo->ioService(), m_appMThIo->ioService()));
        m_sock->setPointer(this);
    }

    bool TcpClient::isAccepted() const
    {
        return m_sock->isAccepted();
    }

    bool TcpClient::isWorking() const
    {
        return m_sock->isWorking();
    }

    bool TcpClient::isConnected() const
    {
        return m_sock->isConnected();
    }

    bool TcpClient::isOpen() const
    {
        return m_sock->isOpen();
    }

    const boost::asio::ip::tcp::endpoint& TcpClient::bindEndpoint()
    {
        return m_sock->bindEndpoint();
    }

    const boost::asio::ip::tcp::endpoint& TcpClient::connectEndpoint()
    {
        return m_sock->connectEndpoint();
    }

    const boost::asio::ip::tcp::endpoint& TcpClient::localEndpoint()
    {
        return m_sock->localEndpoint();
    }

    const boost::asio::ip::tcp::endpoint& TcpClient::remoteEndpoint()
    {
        return m_sock->remoteEndpoint();
    }

    const boost::asio::ip::tcp::endpoint& TcpClient::localEndpointEx()
    {
        return m_sock->localEndpointEx();
    }

    const boost::asio::ip::tcp::endpoint& TcpClient::remoteEndpointEx()
    {
        return m_sock->remoteEndpointEx();
    }

    int TcpClient::connect(const std::string& ip, std::uint16_t port, bool asyncConnect, std::string& eMessage)
    {
        return m_sock->connect(ip, port, asyncConnect, eMessage);
    }

    int TcpClient::connect(const std::string& ip, std::uint16_t port, bool asyncConnect /* = true */)
    {
        return m_sock->connect(ip, port, asyncConnect);
    }

    void TcpClient::close()
    {
        m_sock->close();
    }

    int TcpClient::sendAsync(const char* data, std::uint32_t size, std::string& eMeseage)
    {
        return m_sock->sendAsync(data, size, eMeseage);
    }

    int TcpClient::sendAsync(const char* data, std::uint32_t size)
    {
        return m_sock->sendAsync(data, size);
    }

    int TcpClient::sendSynch(const char* data, std::uint32_t size, int&bytesTransferred, std::string& eMeseage)
    {
        return m_sock->sendSynch(data, size, bytesTransferred, eMeseage);
    }
};


#endif//xx_impl_H
