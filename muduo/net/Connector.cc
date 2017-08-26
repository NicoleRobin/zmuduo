#include <muduo/net/Connector.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>

using namespace muduo;
using namespace muduo::net;

const int Connector::kMaRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
	:loop_(loop), 
	serverAddr_(serverAddr), 
	connect_(false), 
	state_(kDissconnected), 
	retryDelayMs_(kInitRetryDelayMs)
{
	LOG_DEBUG << "ctor[" << this << "]";
}


