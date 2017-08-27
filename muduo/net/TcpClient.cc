#include <muduo/net/TcpClient.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Connector.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

namespace muduo
{
	namespace net
	{
		namespace detail
		{
			void removeConnection(EventLoop *loop, const TcpConnectionPtr& conn)
			{
				loop->queueInLoop(boost::bind(&TcpConnection::connectionDestroyed, conn));
			}

			void removeConnector(const ConnectorPtr &connector)
			{
			}
		}
	}
}

TcpClient::TcpClient(EventLoop *loop, 
		const InetAddress &serverAddr, 
		const string &nameArg)
	:loop_(CHECK_NOTNULL(loop)), 
	connector_(new Connector(loop, serverAddr)), 
	name_(nameArg), 
	connectionCallback_(defaultConnectionCallback), 
	messageCallback_(defaultMessageCallback), 
	retry_(false), 
	connect_(true), 
	nextConnId_(1)
{
	connector_->setNewConnectionCallback(boost::bind(&TcpClient::newConnection, this, _1));
	LOG_INFO << "TcpClient::TcpClient[" << name_ 
		<< "] - connector " << get_pointer(connector);
}

TcpClient::~TcpClient()
{
	LOG_INFO << "TcpClient::~TcpClient[" << name_ 
		<< "] - connector " << get_pointer(connector_);
	TcpConnectionPtr conn;
	bool unique = false;
	{
		MutexLockGuard lock(mutex_);
		unique = connection_.unique();
		conn = connection_;
	}
	if (conn)
	{
		assert(loop_ == conn->getLoop());
		CloseCallback cb = boost::bind(&detail::removeConnection, loop_, _1);
		loop_->runInLoop(boost::bind(&TcpConnection::setCloseCallback, conn, cb));
		if (unique)
		{
			conn->forceClose();
		}
	}
	else
	{
		connector_->stop();
		loop_->runAfter(1, boost::bind(&detail::removeConnector, connector_));
	}
}

// 建立连接
void TcpClient::connect()
{
	LOG_INFO << "TcpClient::connect[" << name_ << "] - connection to " 
		<< connector_->serverAddress().toIpPort();
	connect_ = true;
	connector_->start();
}

void TcpClient::disconnect()
{
	connect_ = false;
	{
		MutexLockGuard lock(mutex_);
		if (connection_)
		{
			connection_->shutdown();
		}
	}
}

void TcpClient::stop()
{
	connect_ = false;
	connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
	loop_->assertInLoopThread();
	InetAddress peerAddr(sockets::getPeerAddr(sockfd));
	char buf[32];
	snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
	++nextConnId_;
	string connName = name_ + buf;

	InetAddress localAddr(sockets::getLoclAddr(sockfd));
	TcpConnectionPtr conn(new TcpConnectiond(loop_, connName, sockfd, 
				localAddr, peerAddr));

	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(boost::bind(&TcpClient::removeConnection, this, _1));

	{
		MutexLockGuard lock(mutex_);
		connection_ = conn;
	}
	conn->connectionEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());

	{
		MutexLockGuard lock(mutex_);
		assert(connection_ == conn);
		connection_.reset();
	}

	loop_->queueInLoop(boost::bind(&TcpConnection::connect(connectDestroyed, conn)));
	if (retry_ && connect_)
	{
		LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to " 
			<< connector_->serverAddress().toIpPort();
		connector_->reset();
	}
}
