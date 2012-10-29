#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "thread.h"

struct posal_thread {
	pthread_t pthread;
	struct {
		void (* fn)(void *);
		void *data;
	} params;
};

static void *__posal_thread_fn(void *pdata)
{
	struct posal_thread *thread = (struct posal_thread *)pdata;

	thread->params.fn(thread->params.data);

	return NULL;
}

int posal_thread_create(posal_thread_t *thd, void (*fn)(void *), void *data)
{
	struct posal_thread *ret;
	pthread_attr_t attr;
	int rc;

	if (fn == NULL)
		return -EINVAL;

	ret = (struct posal_thread *)malloc(sizeof(struct posal_thread));
	if (ret == NULL)
		return -ENOMEM;

	ret->params.fn = fn;
	ret->params.data = data;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(&ret->pthread, &attr, __posal_thread_fn, ret);
	pthread_attr_destroy(&attr);
	if (rc) {
		free(ret);
		return -rc;
	}

	*thd = (posal_thread_t)ret;

	return 0;
}

void posal_thread_destroy(posal_thread_t thd)
{
	if (thd == NULL)
		return;

	pthread_kill(thd->pthread, SIGHUP);

	free(thd);
}

void posal_thread_join(posal_thread_t thd)
{
	if (thd == NULL)
		return;

	pthread_join(thd->pthread, NULL);

	free(thd);
}

int posal_sem_create(posal_sem_t *sem, int initialValue)
{
	sem_t *sph;
	int rc;

	sph = (sem_t *)malloc(sizeof(sem_t));
	if (sph == NULL)
		return -ENOMEM;

	rc = sem_init(sph, 0, initialValue);
	if (rc) {
		free(sph);
		return -errno;
	}

	*sem = (posal_sem_t)sph;

	return 0;
}

int posal_sem_post(posal_sem_t sem)
{
	int rc;

	rc = sem_post((sem_t *)sem);
	if (rc)
		return -errno;

	return 0;
}

int posal_sem_wait(posal_sem_t sem)
{
	int rc;

	rc = sem_wait((sem_t *)sem);
	if (rc)
		return -errno;

	return 0;
}

int posal_sem_timedwait(posal_sem_t sem, unsigned int ms)
{
	struct timespec ts;
	struct timespec dts;
	struct timespec sts;
	int s;

	if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
		return -1;

	dts.tv_sec = ms / 1000;
	dts.tv_nsec = (ms % 1000) * 1000000;
	sts.tv_sec = ts.tv_sec + dts.tv_sec + (dts.tv_nsec + ts.tv_nsec) / 1000000000;
	sts.tv_nsec = (dts.tv_nsec + ts.tv_nsec) % 1000000000;

	while ((s = sem_timedwait((sem_t *)sem, &sts)) == -1 && errno == EINTR)
		continue;
	if (s == -1)
		return -errno;

	return 0;
}

void posal_sem_destroy(posal_sem_t sem)
{
	sem_destroy((sem_t *)sem);
	free(sem);
}

int posal_mutex_create(posal_mutex_t *mtx)
{
	pthread_mutex_t *mutex;
	int rc;

	mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if (mutex == NULL)
		return -ENOMEM;

	rc = pthread_mutex_init(mutex, NULL);
	if (rc) {
		free(mutex);
		return -rc;
	}

	*mtx = (posal_mutex_t)mutex;

	return 0;
}

void posal_mutex_destroy(posal_mutex_t mtx)
{
	pthread_mutex_destroy((pthread_mutex_t *)mtx);
	free(mtx);
}

int posal_mutex_lock(posal_mutex_t mtx)
{
	int rc;

	rc = pthread_mutex_lock((pthread_mutex_t *)mtx);
	if (rc)
		return -rc;

	return 0;
}

int posal_mutex_unlock(posal_mutex_t mtx)
{
	int rc;

	rc = pthread_mutex_unlock((pthread_mutex_t *)mtx);
	if (rc)
		return -rc;

	return 0;
}

Semaphore::Semaphore(unsigned int initialValue)
{
	posal_sem_create(&m_sem, initialValue);
}

Semaphore::~Semaphore()
{
	posal_sem_destroy(m_sem);
}

int Semaphore::post(void)
{
	return posal_sem_post(m_sem);
}

int Semaphore::wait(void)
{
	return posal_sem_wait(m_sem);
}

int Semaphore::wait(unsigned int ms)
{
	return posal_sem_timedwait(m_sem, ms);
}

static void __posal_mm_thread_fn(void *data)
{
	Runnable *r = static_cast<Runnable *>(data);

	r->run();
}

Thread::Thread(void (*fn)(void *), void *data)
 : m_finished(false)
{
	posal_thread_create(&m_thread, fn, data);
}

Thread::Thread(Runnable &runnable)
 : m_finished(false)
{
	posal_thread_create(&m_thread, __posal_mm_thread_fn, &runnable);
}

Thread::~Thread()
{
	if (m_finished == false)
		posal_thread_destroy(m_thread);
}

void Thread::join(void)
{
	posal_thread_join(m_thread);
	m_finished = true;
}

Mutex::Mutex()
{
	posal_mutex_create(&m_mtx);
}

Mutex::~Mutex()
{
	posal_mutex_destroy(m_mtx);
}

int Mutex::lock(void)
{
	return posal_mutex_lock(m_mtx);
}

int Mutex::unlock(void)
{
	return posal_mutex_unlock(m_mtx);
}

ScopedMutex::ScopedMutex(Mutex &mutex)
 : m_mtx(mutex)
{
	m_mtx.lock();
}

ScopedMutex::~ScopedMutex()
{
	m_mtx.unlock();
}
