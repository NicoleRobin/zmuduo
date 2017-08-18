#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>

#include <boost/any.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.h>
#include <boost/shared_ptr.hpp>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo
{
	namespace net
	{
		class Channel;
		class EventLoop;
		class Socket;

		// Tcp connection, for both client and server usage.
		// This is an interface class, so don't expose too much details
		class TcpConnection : boost::noncopyable, public boost::enable_shared_from_this<TcpConnection>
		{
		public:
			// Constructs a TcpConnection with a connected sockfd
			// User should not create this object
			TcpConnection(EventLoop *loop,
					const string &name, 
					int sockfd, 
					const InetAddress &localAddr, 
					const InetAddress &peerAddr);
			~TcpConnection();

			EventLoop* getLoop() const { return loop_; }
			const string& name() const { return name_; }
			const InetAddress& localAddress() const { return localAddr_; }
			const InetAddress& peerAddress() const { return peerAddr_; }
			bool connected() const { return state_ == kConnected; }
			bool disconnected() const { return state_ == kDisconnected; }
			// return true if success
			bool getTcpInfo(struct tcp_info*) const;
			string getTcpInfoString() const;

			void send(const void *message, int len);
			void send(const StringPiece &message);
			void send(Buffer *message);
			void shutdown();
			void forceClose();
			void forceCloseWithDelay(double seconds);
			void setTcpNoDelay(bool on);
			void startRead();
			void stopRead();
			bool isReading() const { return reading_; }
			
			void setContext(const boost::any &context) { context_ = context; }
			const boost::any& getContext() const { return context_; }
			boost::andy& getMutableContext() { return context_; }

			void setConnectionCallback(const ConnectionCallback &cb)
			{ connectionCallback_ = cb; }

			void setMessageCallback(const MessageCallback &cb)
			{ messageCallback_ = cb; }

			void setWriteCompleteCallback(const WriteCompleteCallback &cb)
			{ writeCompleteCallback_ = cb; }

			void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
			{ highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

			// Advanced interface
			Buffer* inputBuffer()
			{ return &inputBuffer_; }

			Buffer* outputBuffer()
			{ return &outputBuffer_; }

			// Internal use only
			void setCloseCallback(const CloseCallback &cb)
			{ closeCallback_ = cb; }

			// called when TcpServer accepts a new connection
			void connectEstablished();
			// called when TcpServer has removed me from its map
			void connectDestroyed();

		private:
			enum StateE { kDisconnected, kConnecting, kConnected, kDissconnecting };
			void handleRead(TimerStamp receiveTime);
			void handleWrite();
			void handleClose();
			void handleError();

			void sendInLoop(const StringPiece &message);
			void sendInLoop(const void *message, size_t len);
			void shutdownInLoop();

			void forceCloseInLoop();
			void setState(StateE s) { state_ = s; }
			const char* stateToString() const;
			void startReadInLoop();
			void stopReadInLoop();

			EventLoop *loop_;
			const string name_;
			StateE state_;
			bool reading_;

			boost::scoped_ptr<Socket> socket_;
			boolst::soped_ptr<Channel> channel_;
			const InetAddress localAddr_;
			const InetAddress peerAddr_;
			ConnectionCallback connectionCallback_;
			MessageCallback messageCallback_;
			WriteCompleteCallback writeCompleteCallback_;
			HighWaterMarkCallback highWaterMarkCallback_;
			CloseCallback closeCallback_;
			size_t highWaterMark_;
			Buffer inputBuffer_;
			Buffer outputBuffer_;
			boost::any context_;
		};
		typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;
	}
}

#endif /* MUDUO_NET_TCPCONNECTION_H */
