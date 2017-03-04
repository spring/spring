/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

namespace spring {
	class noncopyable
	{
	protected:
		noncopyable() {}
		~noncopyable() {}
	private:
		noncopyable(const noncopyable&);
		const noncopyable& operator=(const noncopyable&);
	};
}

#endif // NONCOPYABLE_H
