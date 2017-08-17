#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include <map>
#include <verctor>
#include <boost/noncopyable.h>

#include <muduo/base/Timestamp.h>
#include <muduo/net/EventLoop.h>

namespace muduo
{
	namespace net
	{
		class Channel;
		
		// Base class for IO Multiplexing
		// This class doesn't own the channel object
		class Poller : boost::noncopyable
		{
		public:
			typedef std::vector<Channel*> ChannelList;
			
			Poller(EventLoop *loop);
			virtual ~Poller();

			// Poll the I/O events
			// Must be called in the loop thread, why?
			virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;

			// Changes the interested I/O events
			// Must be called in the loop thead, why?
			virtual void updateChannel(Channel *channel) = 0;

			// Remove the channel, when it destructs
			// Must be called in the loop thread
			virtual void removeChannel(Channel *channel) = 0;

			virtual bool hasChannel(Channel *channel) const;

			static Poller* newDefaultPoller(EventLoop *loop);

			void assertInLoopThread() const
			{
				ownerLoop_->assertInLoopThread();
			}

		protected:
			typedef std::map<int, Channel*> ChannelMap;
			ChannelMap channels_;

		private:
			EventLoop *ownerLoop_;
		};
	}
}

#endif /* MUDUO_NET_POLLER_H */
