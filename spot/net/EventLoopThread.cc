#include <spot/net/EventLoopThread.h>

#include <spot/net/EventLoop.h>


using namespace spot;
using namespace spot::net;


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
	const string& name)
	: loop_(NULL),
	exiting_(false),
	thread_(std::bind(&EventLoopThread::threadFunc, this), name),
	mutex_(),
	cond_(mutex_),
	callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
	exiting_ = true;
	if (loop_ != NULL) // not 100% race-free, eg. threadFunc could be running callback_.
	{
		// still a tiny chance to call destructed object, if threadFunc exits just now.
		// but when EventLoopThread destructs, usually programming is exiting anyway.
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::startLoop()
{
	assert(!thread_.started());
	thread_.start();

	{
		MutexLockGuard lock(mutex_);
		while (loop_ == NULL)
		{
			cond_.wait();
		}
	}

	return loop_;
}

void EventLoopThread::threadFunc()
{
	EventLoop loop;

	if (callback_)
	{
		callback_(&loop);
	}

	{
		MutexLockGuard lock(mutex_);
		loop_ = &loop;
		cond_.notify();
	}

	loop.loop();
	loop_ = NULL;
}

void EventLoopThread::join()
{
	thread_.join();
	return;
}

void EventLoopThread::stop()
{
	loop_->quit();
	return;
}
