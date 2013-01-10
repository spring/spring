#!/bin/sh
set +e

# run CppCheck for the engine sources
find rts/ \
	-name "*\.cpp" -or -name "*\.c" \
	| grep -v '^rts/lib' \
	| cppcheck --file-list=/dev/stdin \
		--force --quiet \
		--suppressions-list=buildbot/slave/cppcheck/cppcheck.supress \
		--enable=style,information \
		-I rts \
		$@

# run CppCheck for the native AI sources
find AI/ \
	-name "*\.cpp" -or -name "*\.c" \
	| cppcheck --file-list=/dev/stdin \
		--force --quiet \
		--suppressions-list=buildbot/slave/cppcheck/cppcheck.supress \
		--enable=style,information \
		-I rts -I rts/ExternalAI/Interface -I AI/Wrappers \
		$@
