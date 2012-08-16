#ifndef SCOPED_FPU_SETTINGS
#define SCOPED_FPU_SETTINGS

// dedicated is compiled w/o streflop!
#if defined(__SUPPORT_SNAN__) && !defined(DEDICATED)

class ScopedDisableFpuExceptions {
public:
	ScopedDisableFpuExceptions() {
		if (!GML::Enabled() || Threading::IsSimThread()) {
			streflop::fegetenv(&fenv);
			streflop::feclearexcept(streflop::FPU_Exceptions(streflop::FE_INVALID | streflop::FE_DIVBYZERO | streflop::FE_OVERFLOW));
		}
	}
	~ScopedDisableFpuExceptions() {
		if (!GML::Enabled() || Threading::IsSimThread())
			streflop::fesetenv(&fenv);
	}
private:
	streflop::fpenv_t fenv;
};

#else

class ScopedDisableFpuExceptions {};

#endif

#endif // SCOPED_FPU_SETTINGS
