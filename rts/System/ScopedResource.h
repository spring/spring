#pragma once

namespace spring {
	template<typename D>
	class ScopedNullResource {
	public:
		template<typename C>
		ScopedNullResource(C&& c, D&& d_)
			: d{ std::move(d_) }
		{
			c();
		}
		~ScopedNullResource() {
			d();
		}
	private:
		D&& d;
	};

	template<typename R, typename D>
	class ScopedResource {
	public:
		ScopedResource(R&& r_, D&& d_)
			: r{ std::move(r_) }
			, d{ std::move(d_) }
		{}
		~ScopedResource() {
			d(r);
		}
		const R& operator()() const { return r; }
		      R& operator()()       { return r; }
	private:
		R&& r;
		D&& d;
	};
}