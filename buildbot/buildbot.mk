# Author: Tobi Vollebregt
#
# Makefile to speed up some Spring buildbot related tasks.
#
# NOTE: renamed to buildbot.mk in the repository, because Makefile is in .gitignore
#       (with good reason, as long as we allow in-source builds)
#

.PHONY : start stop start-master stop-master reload
.PHONY : start-slave stop-slave enter-chroot
.PHONY : start-github stop-github
.PHONY : start-stacktrace-translator stop-stacktrace-translator

start-master:
	env -i PATH=$$PATH buildbot start master

stop-master:
	buildbot stop master

start-slave:
	##### Not using schroot
	#env -i PATH=$$PATH nice -19 ionice -c3 buildbot start slaves/testslave
	##### Using schroot
	schroot --begin-session --chroot buildbot-lucid > ~/run/slave_schroot_session
	## `ionice -c3' sets the process to idle IO priority.
	## This is only useful when using CFQ IO scheduler on the relevant disk.
	schroot --run-session --chroot `cat ~/run/slave_schroot_session` -- env -i PATH=/usr/local/bin:/usr/bin:/bin nice -19 ionice -c3 buildbot start /slave

stop-slave:
	##### Not using schroot
	#buildbot stop slaves/testslave
	##### Using schroot
	schroot --run-session --chroot `cat ~/run/slave_schroot_session` buildbot stop /slave
	schroot --end-session --chroot `cat ~/run/slave_schroot_session`
	rm ~/run/slave_schroot_session

enter-chroot:
	schroot --run-session --chroot `cat ~/run/slave_schroot_session`

start-github:
	env -i PATH=$$PATH buildbot/contrib/github_buildbot.py -m localhost:9989 -p 9987 -l ~/log/github_buildbot.log -L debug --pidfile ~/run/github_buildbot.pid &

stop-github:
	kill `cat ~/run/github_buildbot.pid`

start-stacktrace-translator:
	spring/buildbot/stacktrace_translator.py >> ~/log/stacktrace_translator.log 2>&1 &

stop-stacktrace-translator:
	kill `cat ~/run/stacktrace_translator.pid`

start: start-master start-slave start-github

stop: stop-github stop-slave stop-master

reload:
	buildbot sighup master
