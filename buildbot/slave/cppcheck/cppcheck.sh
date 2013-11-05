#!/bin/sh
set +e

CHECK="cppcheck --file-list=/dev/stdin --force --quiet \
	--suppressions-list=buildbot/slave/cppcheck/cppcheck.supress \
	--enable=performance,portability \
	--std=c++11"
# run CppCheck for the engine sources
find rts/ \
	-name "*\.cpp" -or -name "*\.c" \
	| grep -v '^rts/lib' \
	| $CHECK \
		-I rts \
		$@

# run CppCheck for the native AI sources
find AI/ \
	-name "*\.cpp" -or -name "*\.c" \
	| $CHECK \
		-I rts -I rts/ExternalAI/Interface -I AI/Wrappers \
		$@
