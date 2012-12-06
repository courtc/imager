#pragma once

#include "thread.h"

class Animation {
public:
	Animation(double from, double to, Timestamp dur)
	{
		m_start = Time::MS();
		m_end   = m_start + dur;
		m_from  = from;
		m_to    = to;
	}

	virtual ~Animation() { }

	void tick(Timestamp now)
	{
		double val =	(((double)now  - m_start) /
				((double)m_end - m_start)) *
				(m_to - m_from) + m_from;
		update(val);
	}

	virtual void update(double value) = 0;
	virtual bool finished(void) const { return false; }

	Timestamp getStart(void) const
	{
		return m_start;
	}

	Timestamp getEnd(void) const
	{
		return m_end;
	}

private:
	Timestamp m_start;
	Timestamp m_end;
	double m_from;
	double m_to;
};

class Animator {
public:

	void add(Animation *anim)
	{
		m_animations.push_back(anim);
	}

	void remove(Animation *anim)
	{
		m_animations.remove(anim);
	}

	void step(void)
	{
		std::list<Animation *>::iterator it = m_animations.begin();
		Timestamp now = Time::MS();

		for (; it != m_animations.end(); ++it) {
			(*it)->tick(now);
		}
	}

private:
	std::list<Animation *> m_animations;
};
