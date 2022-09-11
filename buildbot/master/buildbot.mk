# Author: Tobi Vollebregt
#
# Makefile to speed up some Spring buildbot related tasks.
#
# NOTE: renamed to buildbot.mk in the repository, because Makefile is in .gitignore
#       (with good reason, as long as we allow in-source builds)
#

.PHONY : start stop
.PHONY : start-master stop-master reload checkconfig
.PHONY : start-stacktrace-translator stop-stacktrace-translator

start-stacktrace-translator:
	spring/buildbot/stacktrace_translator/stacktrace_translator.py >> ~/log/stacktrace_translator.log 2>&1 &

stop-stacktrace-translator:
	-[ -e ~/run/stacktrace_translator.pid ] && kill `cat ~/run/stacktrace_translator.pid`
	rm -f ~/run/stacktrace_translator.pid

start: start-stacktrace-translator

stop: stop-stacktrace-translator

