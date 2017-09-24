#!/usr/bin/python
# -*- encoding: windows-1250 -*-

"""Annotate memleaks.log from mmgr that have a numerical backtrace with text backtrace.

Alloc.   Addr       Size       Addr       Size                         BreakOn  BreakOn              
Number Reported   Reported    Actual     Actual     Unused    Method   Dealloc  Realloc Allocated by                                    Backtrace (optional)
------ ---------- ---------- ---------- ---------- ---------- -------- -------  ------- -----------------------------------------------------------------------------------------------------------
012345 0x0D200530 0x00000008 0x0D2004B0 0x00000108 0x00000000 new         N        N    ??(00000)::??	 004834da 00535d62 004cef1f 004d7fba 004d8617 007c9c5e 00804a5d 0042749a 0042ef34 0041e28d"""

import os, sys
from subprocess import Popen, PIPE
import re

__author__ = "Marek Baczyński <imbaczek@gmail.com>"
__license__ = "GPLv2"

CMD_GDB = "gdb -q %s"
CMD_ADDR2LINE = "addr2line -f -i -e %s"
CMD_CPPFILT = "c++filt"

def call_cppfilt(sym):
    p = Popen((CMD_CPPFILT).split(), bufsize=1024,
            stdin=PIPE, stdout=PIPE)
    stdout, stderr = p.communicate(sym)
    return stdout

cache_hits = 0
def try_in_cache(bt, cache):
    global cache_hits
    result = []
    for addr in bt:
        if addr not in cache:
            return None
        else:
            result.append(cache[addr])
    cache_hits += 1
    return result

_addr2linecache = {}
def process_bt_addr2line(bt):
    cached = try_in_cache(bt, _addr2linecache)
    if cached:
        return cached
    addr2line = Popen((CMD_ADDR2LINE%sys.argv[1]).split(), bufsize=1024,
            stdin=PIPE, stdout=PIPE)
    stdout, stderr = addr2line.communicate('\n'.join(bt))
    lines = stdout.splitlines()
    result = []
    for i in xrange(0, len(lines), 2):
        sym = lines[i]
        fileline = lines[i+1]
        if sym != "??" and sym.startswith('_Z'):
            sym = call_cppfilt('_'+sym)
        result.append((bt[i//2], fileline, sym))
        _addr2linecache[bt[i//2]] = result[-1]
    return result

_gdbcache = {}
_regdb = re.compile(r'^Line (?P<line>\d+) of "(?P<filename>[\w\\/\.\-+:]*)" starts at address\s+0x[0-9a-f]+ <(?P<sym>\w+)\+\d+> and ends at 0x[0-9a-f]+ <(?P<symend>[^+]+)(\+\d+)?>.')
_regdb_noinfo = re.compile(r'^No line number information available for address\s+0x[0-9a-f]+ <(?P<sym>\w+)\+\d+>')
def process_bt_gdb(bt):
    cached = try_in_cache(bt, _gdbcache)
    if cached:
        return cached
    commands = ['info line *%s'%addr for addr in bt]
    gdb = Popen((CMD_GDB%sys.argv[1]).split() + ['--batch'] + \
                ['--eval-command=%s'%cmd for cmd in commands],
            bufsize=1024, stdin=PIPE, stdout=PIPE)
    stdout, stderr = gdb.communicate()
    lines = []
    for line in stdout.splitlines():
        if line.startswith((' ', '\t')):
            lines[-1] += " "+line.strip()
        else:
            lines.append(line)
    result = []
    for i, line in enumerate(lines):
        m = _regdb.match(line)
        if m:
            fileline = "%s:%s"%(m.group('filename'), m.group('line'))
        else:
            m = _regdb_noinfo.match(line)
            fileline = "????:??"
        if not m:
            print '\t%s'%line
            continue
        sym = m.group('sym')
        if sym != "??" and sym.startswith('_Z'):
            sym = call_cppfilt('_'+sym)
        result.append((bt[i], fileline, sym))
        _gdbcache[bt[i]] = result[-1]
    return result


def parse_line(line, usegdb=False):
    if not line.startswith(tuple('0123456789')):
        return False
    if "memory leaks found:" in line:
        return False
    fields = line.split()
    bt = [hex(int(f, 16)-4) for f in fields[10:]]
    if not usegdb:
        for addr, fileline, sym in process_bt_addr2line(bt):
            print '\t%-12s  %s\t\t%s'%(addr, fileline, sym)
    else:
        for addr, fileline, sym in process_bt_gdb(bt):
            print '\t%-12s  %-40s  %s'%(addr, fileline, sym)
    return True


def main():
    addr2line = Popen((CMD_ADDR2LINE%sys.argv[1]).split(), bufsize=1024,
            stdin=PIPE, stdout=PIPE)
    count = 0
    for line in file('memleaks.log'):
        print line.strip()
        if parse_line(line, usegdb=True):
            count += 1
            print
    print '%s total lines annotated'%count
    print '%s cache hits'%cache_hits


if __name__ == '__main__':
    main()
