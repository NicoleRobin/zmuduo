#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopyable.h>

#include <muduo/net/Channel.h>
#include <muduo/net/Socket.h>

namespace muduo
{
	namespace net
	{
		class EventLoop;
		class InetAddress;

		// Acceptor of incoming Tcp connection
		class Accept : boost::noncopyable
		{
		public:
			typedef boost::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

			Accecptor(EventLoop *loop, const InetAddress &listAddr, bool reuseport);
			~Accecptor();

			void setNewConnetcionCall(const NewConnectionCallback &cb)
			{ newConnectionCallback_ = cb; }

			bool listenning() const { return listenning_; }
			void listen();

		private:
			void handleRead();

			EventLoop *loop_;
			Socket acceptSocket_;
			Channel acceptChannel_;
			NewConnectionCallback newConnectionCallback_;
			bool listening_;
			int idleFd_;
		};
	}
}

#endif /* MUDUO_NET_ACCEPTOR_H */
