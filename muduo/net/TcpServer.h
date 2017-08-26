#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Type.h>
#include <muduo/net/TcpConnection.h>

#include <map>
#include <boost/noncopyable.h>
#include <boost/scoped_ptr.h>
#include <boost/shared_ptr.h>

namespace muduo
{
	namespace net
	{
		class Acceptor;
		class EventLoop;
		class EventLoopThreadPool;

		// Tcp server, supports single-threaded and thread-pool modeles.
		// This is an interface class, so don't expose too much detailes.
		class TcpServer : boost::noncopyable
		{
		public:
			typedef boost::function<void(EventLoop*)> ThreadInitCallback;
			enum Option
			{
				kNoReusePort;
				kReusePort;
			};

			TcpServer(EventLoop *loop,
					const InetAddress &listenAddr, 
					const string &nameArg, 
					Option option = kNoReusePort);
			~TcpServer();
			
			const string& ipPort() const { return ipPort_; }
			const string& name() const { return name_; }
			EvnetLoop* getLoop() const { return loop_; }

			// Set the number of threads for handling input.
			// Always accepts new connection in loop's thread.
			// Must be called before @c start
			// $param numThreads
			// - 0 means all I/O in loop's thread, no thread will created.
			// this is the default value.
			// - 1 means all I/O in another thread.
			// - n means a thread pool with N threads, new connections 
			// are assigned on a round-robin basis.
			void setThreadNum(int numThreads);
			void setThreadInitCallback(const ThreadInitCallback &cb)
			{ threadInitCallback_ = cb; }
			// valid after calling start()
			boost::shared_ptr<EventLoopThreadPool> threadPool()
			{ return threadPool_; }

			// Start the server if it's not listening.
			// It's harmless to call it multiple times.
			// Thread safe.
			void start();

			// Set connection callback
			// Not thread safe
			void setConnectionCallback(const ConnectionCallback &cb)
			{ connectionCallback_ = cb; }

			// Set message callback
			// Not thread safe
			void setMessageCallback(const MessageCallback &cb)
			{ messageCallback_ = cb; }

			// Set write commplete callback
			// Not thread safe
			void setWriteCompleteCallback(const WriteCompleteCallback &cb)
			{ writeCompleteCallback_ = cb; }

		private:
			// Not thread safe, but in loop
			void newConnection(int sockfd, const InetAddress &peerAddr);
			// Thread safe
			void removeConnection(const TcpConnection &conn);
			// Not thread safe, but in loop
			void removeConnectionInLoop(const TcpConnection &conn);

			typedef std::map<string, TcpConnectionPtr> ConnectionMap;

			EventLoop *loop_;
			const string ipPort_;
			const string name_;
			boost::scoped_ptr<Acceptor> acceptor_;
			boost::shared_ptr<EventLoopThreadPool> threadPool_;
			ConnectionCallback connectionCallback_;
			MessageCallback messageCallback_;
			WriteCompleteCallback writeCompleteCallback_;
			ThreadInitCallback threadInitCallback_;
			AtomicInt32 started_;
			// always in loop thread
			int nextConnId_;
			ConnectionMap connections_;
		};
	}
}

#endif /* MUDUO_NET_TCPSERVER_H */
