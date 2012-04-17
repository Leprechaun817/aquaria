#ifndef BBGE_MT_H
#define BBGE_MT_H

#include <cstddef>
#include <queue>
#include <map>

class Lockable
{
public:
	Lockable();
	virtual ~Lockable();
	void lock();
	void unlock();

protected:
	inline void *mutex() { return _mtx; }

private:
	void *_mtx;
};

class Waitable : public Lockable
{
public:
	Waitable();
	virtual ~Waitable();
	void wait(); // releases the associated lock while waiting
	void signal(); // signal a single waiting thread
	void broadcast(); // signal all waiting threads

private:
	void *_cond;
};

class MTGuard
{
public:
	MTGuard(Lockable& x) : _obj(&x) { _obj->lock(); }
	MTGuard(Lockable* x) : _obj(x)  { _obj->lock(); }
	~MTGuard() { _obj->unlock(); }
	Lockable *_obj;
};

template <typename T> class BlockingQueue : public Waitable
{
public:
	void push(const T& e)
	{
		lock();
		_q.push(e);
		unlock();
		signal();
	}
	T pop() // blocks if empty
	{
		lock();
		while(_q.empty())
			wait();
		T ret = _q.front();
		_q.pop();
		unlock();
		return ret;
	}
private:
	std::queue<T> _q;
};

template <typename T> class LockedQueue : public Lockable
{
public:
	void push(const T& e)
	{
		lock();
		_q.push(e);
		unlock();
	}
	T pop()
	{
		lock();
		T ret = _q.empty() ? T() : _q.front();
		_q.pop();
		unlock();
		return ret;
	}
	bool empty()
	{
		lock();
		bool e = _q.empty();
		unlock();
		return e;
	}
	bool popIfPossible(T& e)
	{
		lock();
		if(!_q.empty())
		{
			e = _q.front();
			_q.pop();
			unlock();
			return true;
		}
		unlock();
		return false;
	}

private:
	std::queue<T> _q;
};

// To be distributed to workers in ThreadPool.
class Runnable : public Waitable
{
public:
	Runnable(bool suicide) : _done(false), _shouldquit(false), _suicide(suicide) {};
	virtual ~Runnable() {}

	void run();
	inline void quit() { _shouldquit = true; }
	inline bool shouldQuit() const { return _shouldquit; }
	inline bool isDone() const { return _done; }

	virtual void wait();

protected:
	virtual void _run() = 0; // to be overloaded

private:
	volatile bool _done;
	volatile bool _shouldquit;
	bool _suicide;
};


class FunctionRunnable : public Runnable
{
public:
	typedef void (*Func)(void*);

	FunctionRunnable(bool suicide, Func f, void *ptr = NULL)
		: Runnable(suicide), _func(f), _param1(ptr) {}

	virtual ~FunctionRunnable() {}

protected:
	virtual void _run()
	{
		_func(_param1);
	}

	Func _func;
	void *_param1;
};

#endif
