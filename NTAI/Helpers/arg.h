struct cloneable {};
	struct Cloneable {};

	template<class p_type>
	inline p_type* deep_copy(const p_type* p, const void*) 
	{
		return p ? new p_type(*p) : 0;
	}

	template<class p_type>
	inline p_type* deep_copy(const p_type *p, const cloneable *)
	{
		return p ? p->clone() : 0;
	}

	template<class p_type>
	inline p_type* deep_copy(const p_type *p, const Cloneable *)
	{
		return p ? p->makeClone() : 0;
	}

	template<class p_type>
	inline p_type* deep_copy(const p_type* p) 
	{
		return deep_copy(p, p);
	}

template<typename p_type>
	class grin_ptr
	{
	    // Pointers to utility functions
		typedef void (*delete_ftn)(p_type* p);
	    typedef p_type* (*copy_ftn)(const p_type* p);

	public:
		explicit grin_ptr(p_type* pointee) 
			: do_copy(&my_copy_ftn), p(pointee), do_delete(my_delete_ftn) {}
		
		grin_ptr(const grin_ptr& rhs);
		
		~grin_ptr() throw()              { do_delete(p); }

		const p_type* get() const        { return p; }

		p_type* get()                    { return p; }

		const p_type* operator->() const { return p; }

		p_type* operator->()             { return p; }

		const p_type& operator*() const  { return *p; }

		p_type& operator*()              { return *p; }

		void swap(grin_ptr& with) throw()
			{ p_type* pp = p; p = with.p; with.p = pp; }
		
		grin_ptr& operator=(const grin_ptr& rhs);

	private:
		copy_ftn	do_copy;
		p_type*		p;
		delete_ftn	do_delete;

	    static void my_delete_ftn(p_type* p)
	    {
	    	delete p;
	    }
		
	    static p_type* my_copy_ftn(const p_type* p)
	    {
	    	return deep_copy(p);
	    }
	};