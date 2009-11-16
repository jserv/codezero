#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
import os, sys, shelve, shutil, re
from projpaths import *
from lib import *
from caps import *
from string import Template

cap_strings = { 'ipc' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_IPC | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_IPC_SEND | CAP_IPC_RECV
\t\t\t\t          | CAP_IPC_FULL | CAP_IPC_SHORT
\t\t\t\t          | CAP_IPC_EXTENDED | CAP_CHANGEABLE
\t\t\t\t          | CAP_REPLICABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
'''
, 'tctrl' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_TCTRL | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_TCTRL_CREATE | CAP_TCTRL_DESTROY
\t\t\t\t          | CAP_TCTRL_SUSPEND | CAP_TCTRL_RUN
\t\t\t\t          | CAP_TCTRL_RECYCLE | CAP_TCTRL_WAIT
\t\t\t\t          | CAP_CHANGEABLE | CAP_REPLICABLE
\t\t\t\t          | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
'''
, 'exregs' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_EXREGS | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_EXREGS_RW_PAGER
\t\t\t\t          | CAP_EXREGS_RW_UTCB | CAP_EXREGS_RW_SP
\t\t\t\t          | CAP_EXREGS_RW_PC | CAP_EXREGS_RW_REGS
\t\t\t\t          | CAP_CHANGEABLE | CAP_REPLICABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
'''
, 'capctrl' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_CAP | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_CAP_GRANT | CAP_CAP_READ
\t\t\t\t          | CAP_CAP_SHARE | CAP_CAP_REPLICATE
\t\t\t\t          | CAP_CAP_MODIFY
\t\t\t\t| CAP_CAP_READ | CAP_CAP_SHARE,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
'''
, 'umutex' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_UMUTEX | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_UMUTEX_LOCK | CAP_UMUTEX_UNLOCK,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
'''
, 'threadpool' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY
\t\t\t\t	  | CAP_RTYPE_THREADPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t.size = ${size},
\t\t\t},
'''
, 'spacepool' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_SPACEPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t.size = ${size},
\t\t\t},
'''
, 'cpupool' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_CPUPOOL,
\t\t\t\t.access = 0, .start = 0, .end = 0,
\t\t\t\t.size = ${size} /* Percentage */,
\t\t\t},
'''
, 'mutexpool' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_MUTEXPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t.size = ${size},
\t\t\t},
'''
, 'mappool' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t/* For pmd accounting */
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_MAPPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t/* Function of mem regions, nthreads etc. */
\t\t\t\t.size = ${size},
\t\t\t},
'''
, 'cappool' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t/* For cap spliting, creating, etc. */
\t\t\t\t.target_type = ${target_type},
\t\t\t\t.target = {cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_CAPPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t/* This may be existing caps X 2 etc. */
\t\t\t\t.size = ${size},
\t\t\t},
'''
}

#
# CAP_USE can be one of these:
# CAP_TYPE
# CAPPOOL, CAPCTRL, EXREGS, TCTRL ...
#    CAP_SIZE
#
# _CUSTOM[0-9]
#   CAP_TYPE
# CAP attributes can be one of these:
#
# TARGET_CURRENT_CONT
# TARGET_PAGER_SPACE
# TARGET_ANOTHER_CONT
# TARGET_ANOTHER_PAGER
#
# TARGET_CONTAINER_ID
#

#
# Prepares descriptions of all non-memory capabilities
#
def prepare_capability(cont, param, val):
    # USE makes us assign the initial cap string with blank fields
    if 'USE' in param:
        captype, rest = param.split('_', 1)
        captype = captype.lower()
        cont.caps[captype] = cap_strings[captype]

        # If it is a pool, amend default target type and id
        if captype[-len('pool'):] == 'pool':
            templ = Template(cont.caps[captype])
            cont.caps[captype] = templ.safe_substitute(target_type = 'CURRENT_CONT',
                                                       cid = cont.id)

    # Fill in the blank size field
    elif 'SIZE' in param:
        captype, rest = param.split('_', 1)
        captype = captype.lower()
        templ = Template(cont.caps[captype])
        cont.caps[captype] = templ.safe_substitute(size = val)
        print cont.caps[captype]

    # Fill in capability target type and target id fields
    elif 'TARGET' in param:
        captype, target, ttype = param.split('_', 2)
        captype = captype.lower()
        templ = Template(cont.caps[captype])

        # Target type
        if ttype != None:
            # Insert current container id, if target has current
            if ttype[:len('CURRENT')] == 'CURRENT':
                cont.caps[captype] = templ.safe_substitute(target_type = ttype, cid = cont.id)
            else:
                cont.caps[captype] = templ.safe_substitute(target_type = ttype)
        # Get target value supplied by user
        else:
            cont.caps[captype] = templ.safe_substitute(cid = val)
    else:
        print "No match: ", param

    print captype
    print cont.caps[captype]

'''
        self.threadpool = ''
        self.spacepool = ''
        self.mappool = ''
        self.cappool = ''
        self.mutexpool = ''
        self.tctrl = ''
        self.exregs = ''
        self.capctrl = ''
        self.ipc = ''
        self.custom0 = ''
        self.custom1 = ''
        self.custom2 = ''
        self.custom3 = ''
'''

'''

        elif param[:len('CUSTOM')] == 'CUSTOM':
            matchobj = re.match(r"(CUSTOM){1}([0-9]){1}(\w+)", param)
            prefix, idstr, rest = matchobj.groups()

#
# Prepares descriptions of all non-memory capabilities
#
def prepare_capability(cont, id, param, val):
    if param[:len('THREADPOOL')] == 'THREADPOOL':
        part1, part2 = param.split('_', 1)
        if part1 == "USE" 
    elif param[:len('SPACEPOOL')] == 'SPACEPOOL':
    elif param[:len('MUTEXPOOL')] == 'MUTEXPOOL':
    elif param[:len('CAPPOOL')] == 'CAPPOOL':
    elif param[:len('IPC')] == 'IPC':
    elif param[:len('TCTRL')] == 'TCTRL':
    elif param[:len('EXREGS')] == 'EXREGS':
    elif param[:len('CAPCTRL')] == 'CAPCTRL':
    if param[:len('CUSTOM')] == 'CUSTOM':
'''

