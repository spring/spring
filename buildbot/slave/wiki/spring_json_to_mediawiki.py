#!/usr/bin/python2

# parses the output of spring --list-config-vars from stdin and prints mediawiki, i.e.:
# spring-headless --list-config-vars | ./spring_json_to_mediawiki.py

import json, sys, re
d=json.load(sys.stdin)

for cfgtag in d.iterkeys():
	anchor = "{{taglink|" + cfgtag + "}}"
	rneedle  = r"(.*\w*)" + cfgtag + r"(\w*.*)"
	rh = re.compile(rneedle)
	rreplace = r"\1" + anchor + r"\2"

	for c,t in d.iteritems():
		if c != cfgtag:
			if "description" in t:
				if t['description'].find(cfgtag) >= 0:
					t['description'] = rh.sub(rreplace, t['description'])

for cfgtag,t in sorted(d.iteritems()):
	if cfgtag == 'test':
		continue
	tmp="|name=" + cfgtag
	defval=""
	if "defaultValue" in t:
		defval=t['defaultValue']
	if t['type'] == "std::string":
		tmp += "|type=string"
		tmp += "|defvalue=\"" + str(defval) + "\""
	elif t['type'] == "bool":
		tmp += "|type=" + t['type']
		tmp += "|defvalue=" + ("true" if (defval > 0) else "false")
	else:
		tmp += "|type=" + t['type']
		tmp += "|defvalue=" + str(defval)
	if "minimumValue" in t:
		tmp += "|minvalue=" + str(t['minimumValue'])
	if "maximumValue" in t:
		tmp += "|maxvalue=" + str(t['maximumValue'])
	if "safemodeValue" in t:
		if t['type'] == "bool":
			tmp += "|safevalue=" + ("true" if (t['safemodeValue'] > 0) else "false")
		else:
			tmp += "|safevalue=" + str(t['safemodeValue'])
	else:
		# add empty, so an empty column is shown (w/o this attr the column is hidden)
		tmp += "|safevalue="
	desc=""
	if "description" in t:
		desc = "|description=" + t['description'].encode("utf8")
	else:
		srcfile = re.sub(r'.*rts/(.*)', r'trunk/rts/\1', t['declarationFile'])
		desc = "|description=[" + srcfile.replace("trunk/", "https://github.com/spring/spring/blob/develop/", 1) + "#L" + str(t['declarationLine']) + " source pos]"
	print "{{ConfigValue", tmp, desc, "}}"

