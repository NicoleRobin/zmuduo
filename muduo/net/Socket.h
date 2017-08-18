#ifndef MUDUO_NET_SOCKET_H
#define MUDUO_NET_SOCKET_H

#include <boost/noncopyable.h>

struct tcp_info;

namespace muduo
{
	// TCP networking
	namespace net
	{
		class InetAddress;
		
		// Wraper of socket file descriptor
		// It close the sockfd when desctructs.
		// It's thread safe, all operations are delegated to OS
		class Socket : boost::noncopyable
		{
		public:
			explicit Socket(int sockfd)
				:sockfd_(sockfd)
			{}

			~Socket();

			int fd() const { return sockfd_; }
			// return true if sucess
			bool getTcpInfo(struct tcp_info*) const;
			bool getTcpInfoString(char *buf, int len) const;

			// abort if address in use
			void bindAddress(const InetAddress &localaddr);
			// abort if address in use
			void listen();

			// On success, retruns a non-negative integer that is 
			// a descriptor for the accepted socket, which has been set to 
			// non-blocking and close-on-exec. *peeraddr is assigned.
			// On error, -1 is returned, and *peeraddr is untouched.
			int accept(InetAddress *peeraddr);

			void shutdownWrite();

			// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm)
			void setTcpNoDelay(bool on);

			// Enable/disable SO_REUSEADDR
			void setReuseAddr(bool on);

			// Enable/disable SO_REUSEPORT
			void setReusePort(bool on);

			// Enable/disable SO_KEEPALIVE
			void setKeepAlive(bool on);

		private:
			const int sockfd_;
		};
	}
}

#endif /* MUDUO_NET_SOCKET_H */
