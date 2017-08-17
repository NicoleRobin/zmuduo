#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <muduo/base/Timestamp.h>

namespace muduo
{
	namespace net
	{
		class EventLoop;

		// A selectable I/O channel
		// 
		// This class does't own the file descriptor.
		// The file descriptor could be a socket, 
		// an eventfd, a timefd, or a signalfd
		class Channel : boost::noncopyable
		{
		public:
			typedef boost::function<void()> EventCallback;
			typedef boost::function<void(Timestamp)> ReadEventCallback;
			// why ReadCallback need Timestamp para

			Channel(EventLoop *loop, int fd);
			~Channel();

			void handleEvent(Timestamp receiveTime);
			void setReadCallback(const ReadEventCallback &cb)
			{ readCallback_ = cb; }
			void setWriteCallback(const EventCallback &cb)
			{ writeCallback_ = cb; }
			void setCloseCallback(const EventCallback &cb)
			{ closeCallback_ = cb; }
			void setErrorCallback(const EventCallback &cb)
			{ errorCallback_ = cb; }

			// Tie this channel to the owner object managed by shared_ptr,
			// prevent the owner object being destroyed in handEvent.
			void Tie(const boost::shared_ptr<void>&);

			int fd() const { return fd_; }
			int events() const { return events_; }
			void set_revents(int revt) { revents_ = revt; } // used by poller
			bool isNoneEvent() const { return events_ == KNoneEvent; }

			void enableReading() { events_ |= KReadEvent; update(); }
			void disableReading() { events_ &= ~KReadEvent; update(); }
			void enableWriting() { events_ |= KWriteEvent; update(); }
			void disableWriting() { events_ &= ~KWriteEvent; update(); }
			void disableAll() { events_ = KNoneEvent; update(); }
			bool isWriting() const { return events_ & kWriteEvent; }
			bool isReading() const { return events_ & kReadEvent; }

			// for Poller
			int index() { return index_; }
			void set_index(int idx) { index_ = idx; }

			// for debug
			string reventsToString() const;
			string eventsToString() const;

			void doNotLogHup() { logHup_ = false; }

			EventLoop* ownerLoop() { return loop_; }
			void remove();

		private:
			static string eventsToString(int fd, int ev);
			void update();
			void handleEventWithGuard(Timestamp receiveTime);

			static const int KNoneEvent;
			static const int KReadEvent;
			static const int KWriteEvent;

			EventLoop	*loop_;
			const int	fd_;
			int			events_;
			int			revents_; // it's the received event types of epoll or poll
			int			index_; // used by Poller
			bool		logHup_;

			boost::weak_ptr<void> tie_;
			bool tied_;
			bool eventHandling_;
			bool addedToLoop_;
			ReadEventCallback readCallback_;
			EventCallback writeCallback_;
			EventCallback closeCallback_;
			EventCallback errorCallback_;
		};
	}
}

#endif /* MUDUO_NET_CHANNEL_H */
