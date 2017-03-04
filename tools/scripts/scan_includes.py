#!/usr/bin/python
# -*- encoding: windows-1250 -*-

"""Scan C/C++ files, scan which files get included and output aggregated
counts."""

import os, os.path
import re
from os.path import join
from collections import defaultdict
import operator

__author__ = "Marek Baczyński <imbaczek@gmail.com>"
__license__ = "GPLv2"

def main():
    local = defaultdict(int)
    stdlib = defaultdict(int)
    for root, dirs, files in os.walk('rts'):
        if root.startswith(join('rts', 'lib')):
            continue
        if '.svn' in root:
            continue
        for f in files:
            if any(f.endswith(x) for x in ('.cpp', '.hpp', '.h', '.c')):
                analyze(join(root, f), local, stdlib)
    print '\nSystem headers:'
    print '\n'.join('%s %s'%x for x in sorted(stdlib.items(), key=operator.itemgetter(1)))
    print '\nProject headers:'
    print '\n'.join('%s %s'%x for x in sorted(local.items(), key=operator.itemgetter(1)))

_localinc = re.compile(r'^\s*#\s*include\s+"([\./\\a-zA-Z0-9]+)"')
_systeminc = re.compile(r'^\s*#\s*include\s+<([\./\\a-zA-Z0-9]+)>')

def analyze(filename, local, stdlib):
    print 'processing',filename
    f = file(filename)
    for line in f:
        line = line.strip()
        m = _systeminc.match(line)
        if m:
            stdlib[m.group(1)] += 1
            continue
        m = _localinc.match(line)
        if m:
            local[m.group(1)] += 1
            continue


if __name__ == '__main__':
    main()
