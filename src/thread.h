#pragma once

typedef struct posal_thread * posal_thread_t;
typedef struct posal_sem * posal_sem_t;
typedef struct posal_mutex * posal_mutex_t;

class Runnable {
public:
	virtual void run(void) = 0;
};

class Thread {
public:
	Thread(void (*fn)(void *), void *data);
	Thread(Runnable &runnable);
	~Thread();

	void join(void);

private:
	posal_thread_t m_thread;
	bool m_finished;
};

class Semaphore {
public:
	Semaphore(unsigned int initialValue);
	~Semaphore();

	int post(void);
	int wait(void);
	int wait(unsigned int ms);

private:
	posal_sem_t m_sem;
};

class Mutex {
public:
	Mutex();
	~Mutex();

	int lock(void);
	int unlock(void);

private:
	posal_mutex_t m_mtx;
};

class ScopedMutex {
public:
	ScopedMutex(Mutex &mutex);
	~ScopedMutex();
private:
	Mutex &m_mtx;
};

class Queue {
public:
	class Item {
	public:
		Item *next;
		Item *prev;
	};

	Queue()
	 : head(0), tail(0), nitems(0)
	{ }

	~Queue()
	{ }

	void pushBack(Queue::Item *item)
	{
		lock.lock();
		item->next = 0;
		if (tail != 0) {
			item->prev = tail;
			tail->next = item;
			tail = item;
		} else {
			item->prev = 0;
			tail = head = item;
		}
		nitems++;
		lock.unlock();
	}

	void pushFront(Queue::Item *item)
	{
		lock.lock();
		item->prev = 0;
		if (head != 0) {
			item->next = head;
			head->prev = item;
			head = item;
		} else {
			item->next = 0;
			tail = head = item;
		}
		nitems++;
		lock.unlock();
	}

	int remove(Queue::Item *item)
	{
		int rc = 0;
		lock.lock();
		if (item->prev == 0 && item->next == 0) {
			rc = -1; /* item is not in a list */
		} else {
			if (item->prev != 0)
			item->prev->next = item->next;
			else
				head = item->next;
			if (item->next != 0)
				item->next->prev = item->prev;
			else
				tail = item->prev;
			if (head == 0 || tail == 0)
				head = tail = 0;
			item->next = 0;
			item->prev = 0;
			--nitems;
		}
		lock.unlock();

		return rc;
	}

	Queue::Item *popFront(void)
	{
		Item *ret;
		lock.lock();
		if (head == 0) {
			ret = 0;
		} else {
			ret = head;
			head = head->next;
			if (head == 0)
				tail = 0;
			else
				head->prev = 0;
			ret->next = 0;
			ret->prev = 0;
			nitems--;
		}
		lock.unlock();
		return ret;
	}

	Queue::Item *popBack(void)
	{
		Item *ret;
		lock.lock();
		if (tail == 0) {
			ret = 0;
		} else {
			ret = tail;
			tail = tail->prev;
			if (tail == 0)
				head = 0;
			else
				tail->next = 0;
			ret->next = 0;
			ret->prev = 0;
			nitems--;
		}
		lock.unlock();
		return ret;
	}


	int count(void)
	{
		return nitems;
	}

private:
	Queue::Item *head;
	Queue::Item *tail;
	int nitems;
	Mutex lock;
};

template <typename _T>
class WaitQ {
public:
	class Item : public Queue::Item {
	public:
		Item(_T p)
		 : data(p) { }
		_T data;
	};

	WaitQ(int max = -1)
	{
		sem = new Semaphore(0);
		maxItems = (max == -1) ? 0 : max;
		if (maxItems)
			empty = new Semaphore(max);
	}

	~WaitQ()
	{
		Item *qi;

		delete sem;
		if (maxItems)
			delete empty;

		while ((qi = static_cast<Item *>(queue.popFront())) != 0)
			delete qi;
	}

	void pushFront(_T item)
	{
		if (maxItems)
			empty->wait();

		queue.pushFront(new Item(item));
		sem->post();
	}

	void pushBack(_T item)
	{
		if (maxItems)
			empty->wait();

		queue.pushBack(new Item(item));
		sem->post();
	}


	_T popFront(int timeout_ms = -1)
	{
		Item *qi;
		_T ret;

		if (timeout_ms != -1) {
			if (sem->wait(timeout_ms))
				return 0;
		} else {
			sem->wait();
		}

		qi = static_cast<Item *>(queue.popFront());
		if (qi == 0)
			return 0;

		ret = qi->data;
		delete qi;

		if (maxItems)
			empty->post();

		return ret;
	}

	_T popBack(int timeout_ms = -1)
	{
		Item *qi;
		_T ret;

		if (timeout_ms != -1) {
			if (sem->wait(timeout_ms))
				return 0;
		} else {
			sem->wait();
		}

		qi = static_cast<Item *>(queue.popBack());
		if (qi == 0)
			return 0;

		ret = qi->data;
		delete qi;

		if (maxItems)
			empty->post();

		return ret;
	}


	int count(void)
	{
		return queue.count();
	}

private:
	Queue queue;
	int maxItems;
	Semaphore *sem;
	Semaphore *empty;
};
