#!/usr/bin/env python
#
# Author: Tobi Vollebregt
#
# Very simple script which is ment to be used as SVN post-commit hook to inform
# users of TASServer of commits.  It does this by logging in as a bot and
# sending a summary of the commit (changed paths and log message) to certain
# channels.  It can be configured in the code below.
#
# Usage: commitbot.py REPOS_PATH REVISION
# Example: /usr/local/bin/commitbot.py /var/svn/spring 3604
#

import os
import socket
import sys
import time



class SvnLook:

	svnlook = "/usr/local/bin/svnlook"

	def __init__(self, repos_path, revision):
		self.repos_path = repos_path
		self.revision = revision
		self.author = self.look('author')[0]
		self.changed = self.look('changed')
		self.date = self.look('date')[0]
		self.log = self.look('log')


	def look(self, command):
		return os.popen("%s %s %s --revision %i" % (self.svnlook, command, self.repos_path, self.revision)).read().strip().split('\n')


	def summary(self):
		msg = ["r%i | %s | %s" % (self.revision, self.author, self.date), "Changed paths:"]
		for line in self.changed:
			msg += ["    " + line]
		for line in self.log:
			msg += [line]
		return msg



class TasServer:

	server_address = "taspringmaster.clan-sy.com"
	server_port = 8200

	username = "CommitBot"
	password = ""          # set this to base64 encoded md5 hash of password

	channels = ["commits"]


	def __init__(self):
		self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.socket.connect((self.server_address, self.server_port))
		self.socket.send("LOGIN %s %s 0 * commitbot.py 0\n" % (self.username, self.password))
		for channel in self.channels:
			self.socket.send("JOIN %s\n" % (channel))
		time.sleep(0.1) #prevent ban for spamming


	def send(self, msg):
		for channel in self.channels:
			self.socket.send("SAY %s %s\n" % (channel, msg))
		time.sleep(0.1) #prevent ban for spamming



### main ###

if len(sys.argv) < 3:
	print 'Usage: %s REPOS_PATH REVISION'
	sys.exit(1)

repos_path = sys.argv[1]
revision = int(sys.argv[2])

svnlook = SvnLook(repos_path, revision)
tasserver = TasServer()

for msg in svnlook.summary():
	tasserver.send(msg)
