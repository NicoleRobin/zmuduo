#include <muduo/net/EventLoop.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/Channel.h>
#include <muduo/net/Poller.h>
#include <muduo/net/SocketOps.h>
#include <muduo/net/TimerQueue.h>

#include <boost/bind.hpp>

#include <signal.h>
#include <sys/eventfd.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
	__thread EventLoop *t_loopInThisThread = 0;
	const int kPollTimeMs = 10000;

	int createEventfd()
	{
		int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
		if (eventfd < 0)
		{
			LOG_SYSERR << "failed in eventfd";
			abort();
		}
		return eventfd;
	}

#pragma GCC diagnostic ignored "-Wold-style-cast"
	class IgnoreSigPipe
	{
	public:
		IgnoreSigPipe()
		{
			::signal(SIGPIPE, SIG_IGN);
		}
	};
#pragma GCC diagnostic error "-Wold-style-cast"

	IgnoreSigPipe initObj;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

EventLoop::EventLoop()
	:looping_(false),
	quit_(false),
	eventHandling_(false),
	callingPendingFunctors_(false),
	iteration_(0),
	threadId_(CurrentTherad::tid()),
	poller_(Poller::newDefaultPoller(this)),
	timerQueue_(new TimerQueue(this)),
	wakeupFd_(createEventfd()),
	wakeupChannel_(new Channel(this, wakeupFd_)),
	currentActiveChannel_(NULL)
{
	LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
	if (t_loopInThisThread)
	{
		LOG_FATAL << "Another EventLoop " << t_loopInThisThread
			<< " exists in this thread " << threadId_;
	}
	else
	{
		t_loopInThisThread = this;
	}

	wakeupChannel_->setReadCallback(boost::bind(&EventLoop::handleRead, this));
	// we are always reading the wakeupfd
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
	LOG_DEBUG << "EvengLoop " << this << " of thread " << threadId_ 
		<< " destructs in thread " << CurrentThread::tid();
	wakeupChannel_->disableAll();
	wakeupChannel_->remove();
	::close(wakeupFd_);
	t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
	assert(!looping_);
	assertInLoopThread();
	looping_ = true;
	quit_ = false;
	LOG_TRACE << "EventLoop " << this << " start looping";

	while (!quit_)
	{
		activeChannels_.clear();
		pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
		++iteration_;
		if (Logger::logLevel() <= Logger::TRACE)
		{
			printActiveChannels();
		}

		// Todo sort channel by priority
		eventHanding_ = true;
		for (ChannelList::iterator it = activeChannels_.begin(); 
				it != activeChannels_.end(); ++it)
		{
			currentActiveChannel_ = *it;
			// 调用Channel类的handleEvent方法
			currentActiveChannel_->handleEvent(pollReturnTime_);
		}
		currentActiveChannel_ = NULL;
		eventHandling_ = false;
		doPendingFunctors();
	}

	LOG_TRACE << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void EventLoop::quit()
{
	quit_ = true;
	// There is a chance that loop() just executes while(!quit) and exits,
	// then EventLoop destructs, then we are accessing an invalid object.
	// Can be fixed using mutex_ in both places.
	if (!isInLoopThread())
	{
		wakeup();
	}
}

void EventLoop::runInLoop(const Functor &cb)
{
	if (isInLoopThread())
	{
		cb();
	}
	else
	{
		queueInLoop(cb);
	}
}

void EventLoop::queueInLoop(const Functor &cb)
{
	{
		MutexLockGuard lock(mutex_);
		pendingFunctors_.push_back(cb);
	}

	if (!isInLoopThread() || callingPendingFunctors_)
	{
		wakeup();
	}
}

size_t EventLoop::queueSize() const
{
	MutexLockGuard lock(mutex_);
	return pendingFunctors_.size();
}

TimerId EventLoop::runcAt(const Timestamp &time, const TimerCallback &cb)
{
	return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb)
{
	Timestammp time(addTime(Timerstamp::now(), delay));
	return runAt(time, cb);
}


