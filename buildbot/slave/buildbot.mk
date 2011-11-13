# Author: Tobi Vollebregt
#
# Makefile to speed up some Spring buildbot related tasks.
#
# NOTE: renamed to buildbot.mk in the repository, because Makefile is in .gitignore
#       (with good reason, as long as we allow in-source builds)
#

.PHONY : start stop
.PHONY : start-slave stop-slave enter-chroot

USER:=buildbot

start-slave:
	##### Not using schroot
	#env -i PATH=$$PATH nice -19 ionice -c3 buildslave start slaves/testslave
	##### Using schroot
	schroot --begin-session --chroot buildbot-maverick --user=${USER} > ~/run/slave_schroot_session
	## `ionice -c3' sets the process to idle IO priority.
	## This is only useful when using CFQ IO scheduler on the relevant disk.
	schroot --run-session --chroot `cat ~/run/slave_schroot_session` --user=${USER} -- env -i PATH=/usr/lib/ccache:/usr/local/bin:/usr/bin:/bin nice -19 ionice -c3 buildslave start /slave

stop-slave:
	##### Not using schroot
	#buildbot stop slaves/testslave
	##### Using schroot
	schroot --run-session --chroot `cat ~/run/slave_schroot_session` --user=${USER} buildslave stop /slave
	schroot --end-session --chroot `cat ~/run/slave_schroot_session` --user=${USER}
	rm ~/run/slave_schroot_session

enter-chroot:
	schroot --run-session --chroot `cat ~/run/slave_schroot_session` --user=${USER} --directory /

start: start-slave

stop: stop-slave
