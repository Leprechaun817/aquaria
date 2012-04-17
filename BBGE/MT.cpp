#include "MT.h"
#include "Base.h"


// If more threads are idle than this, start killing excess threads.
// Note: This has nothing to do with the amount of CPU cores.
//       The thread pool will create and destroy threads on demand.
const int spareThreads = 8;


// --------- Lockable ----------

Lockable::Lockable()
: _mtx(NULL)
{
#ifdef BBGE_BUILD_SDL
	_mtx = SDL_CreateMutex();
#endif
}

Lockable::~Lockable()
{
#ifdef BBGE_BUILD_SDL
	SDL_DestroyMutex((SDL_mutex*)_mtx);
#endif
}

void Lockable::lock()
{
#ifdef BBGE_BUILD_SDL
	SDL_LockMutex((SDL_mutex*)_mtx);
#endif
}

void Lockable::unlock()
{
#ifdef BBGE_BUILD_SDL
	SDL_UnlockMutex((SDL_mutex*)_mtx);
#endif
}

// --------- Waitable ----------

Waitable::Waitable()
: _cond(NULL)
{
#ifdef BBGE_BUILD_SDL
	_cond = SDL_CreateCond();
#endif
}

Waitable::~Waitable()
{
#ifdef BBGE_BUILD_SDL
	SDL_DestroyCond((SDL_cond*)_cond);
#endif
}

void Waitable::wait()
{
#ifdef BBGE_BUILD_SDL
	SDL_CondWait((SDL_cond*)_cond, (SDL_mutex*)mutex());
#endif
}

void Waitable::signal()
{
#ifdef BBGE_BUILD_SDL
	SDL_CondSignal((SDL_cond*)_cond);
#endif
}

void Waitable::broadcast()
{
#ifdef BBGE_BUILD_SDL
	SDL_CondBroadcast((SDL_cond*)_cond);
#endif
}

// --------- Runnable ----------

void Runnable::wait()
{
	MTGuard(this);
	while (!_done)
		Waitable::wait();
}

void Runnable::run()
{
	_run(); // this is the job's entry point

	lock();
		_done = true;
		// we may get deleted  by another thread directly after unlock(), have to save this on the stack
		volatile bool suicide = _suicide; 
		broadcast(); // this accesses _cond, and must be done before unlock() too, same reason
	unlock();
	
	if (suicide)
		delete this;
}
