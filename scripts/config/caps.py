#! /usr/bin/env python2.7
# -*- mode: python; coding: utf-8; -*-
import os, sys, shelve, shutil, re
from projpaths import *
from lib import *
from caps import *
from string import Template

cap_strings = { 'ipc' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_IPC | ${target_rtype},
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
, 'irqctrl' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_IRQCTRL | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_IRQCTRL_REGISTER | CAP_IRQCTRL_WAIT
\t\t\t\t          | CAP_CHANGEABLE | CAP_REPLICABLE
\t\t\t\t          | CAP_TRANSFERABLE,
\t\t\t\t.start = IRQ_RANGE_START, .end = IRQ_RANGE_END, .size = 0,
\t\t\t},
'''
, 'exregs' : \
'''
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_EXREGS | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_EXREGS_RW_PAGER
\t\t\t\t          | CAP_EXREGS_RW_UTCB | CAP_EXREGS_RW_SP
\t\t\t\t          | CAP_EXREGS_RW_PC | CAP_EXREGS_RW_REGS
\t\t\t\t          | CAP_CHANGEABLE | CAP_REPLICABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
'''
, 'threadpool' : \
'''
\t\t\t[${idx}] = {
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
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_SPACEPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t.size = ${size},
\t\t\t},
'''
, 'mutexpool' : \
'''
\t\t\t[${idx}] = {
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
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_MAPPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t/* Function of mem regions, nthreads etc. */
\t\t\t\t.size = ${size},
\t\t\t},
'''
}

#
# These are carefully crafted functions, touch with care.
#
def prepare_custom_capability(cont, caplist, param, val):
    if 'TYPE' in param:
        capkey, captype, rest = param.split('_', 2)
        capkey = capkey.lower()
        captype = captype.lower()
        caplist.caps[capkey] = cap_strings[captype]
    elif 'TARGET' in param:
        target_parts = param.split('_', 2)
        if len(target_parts) == 2:
            capkey = target_parts[0].lower()
            templ = Template(caplist.caps[capkey])
            caplist.caps[capkey] = templ.safe_substitute(cid = val)
        elif len(target_parts) == 3:
            capkey = target_parts[0].lower()
            ttype = target_parts[2]
            templ = Template(caplist.caps[capkey])

            # On current container, provide correct rtype and current containerid.
            # Else we leave container id to user-supplied value
            if ttype == 'CURRENT_CONTAINER':
                caplist.caps[capkey] = templ.safe_substitute(target_rtype = 'CAP_RTYPE_CONTAINER',
                                                             cid = cont.id)
            elif ttype == 'CURRENT_PAGER_SPACE':
                caplist.caps[capkey] = templ.safe_substitute(target_rtype = 'CAP_RTYPE_SPACE',
                                                             cid = cont.id)
            elif ttype == 'OTHER_CONTAINER':
                caplist.caps[capkey] = templ.safe_substitute(target_rtype = 'CAP_RTYPE_CONTAINER')
            elif ttype == 'OTHER_PAGER':
                caplist.caps[capkey] = templ.safe_substitute(target_rtype = 'CAP_RTYPE_THREAD')
    else: # Ignore custom_use symbol
        return
    # print capkey
    # print caplist.caps[capkey]

def prepare_typed_capability(cont, caplist, param, val):
    captype, params = param.split('_', 1)
    captype = captype.lower()

    # USE makes us assign the initial cap string with blank fields
    if 'USE' in params:
        caplist.caps[captype] = cap_strings[captype]

        # Prepare string template from capability type
        templ = Template(caplist.caps[captype])

        # If it is a pool, tctrl, exregs, irqctrl, amend current container id as default
        if captype[-len('pool'):] == 'pool' or \
	   captype[-len('irqctrl'):] == 'irqctrl':
            caplist.caps[captype] = templ.safe_substitute(cid = cont.id)

    # Fill in the blank size field
    elif 'SIZE' in params:
        # Get reference to capability string template
        templ = Template(caplist.caps[captype])
        caplist.caps[captype] = templ.safe_substitute(size = val)

    # Fill in capability target type and target id fields
    elif 'TARGET' in params:
        # Get reference to capability string template
        templ = Template(caplist.caps[captype])

        # Two types of strings are expected here: TARGET or TARGET_TARGETTYPE
        # If TARGET, the corresponding value is in val
        target_parts = params.split('_', 1)

        # Target type
        if len(target_parts) == 2:
            ttype = target_parts[1]

            # On current container, provide correct rtype and current containerid.
            # Else we leave container id to user-supplied value
            if ttype == 'CURRENT_CONTAINER':
                caplist.caps[captype] = templ.safe_substitute(target_rtype = 'CAP_RTYPE_CONTAINER',
                                                              cid = cont.id)
            elif ttype == 'CURRENT_PAGER_SPACE':
                caplist.caps[captype] = templ.safe_substitute(target_rtype = 'CAP_RTYPE_SPACE',
                                                              cid = cont.id)
            elif ttype == 'OTHER_CONTAINER':
                caplist.caps[captype] = templ.safe_substitute(target_rtype = 'CAP_RTYPE_CONTAINER')
            elif ttype == 'OTHER_PAGER':
                caplist.caps[captype] = templ.safe_substitute(target_rtype = 'CAP_RTYPE_THREAD')

        # Get target value supplied by user in val
        else:
            caplist.caps[captype] = templ.safe_substitute(cid = val)

    # print captype
    # print caplist.caps[captype]

def prepare_capability(cont, cap_owner, param, val):
    caplist = cont.caplist[cap_owner] # Passing either pager or container caplist.
    if 'CUSTOM' in param:
        prepare_custom_capability(cont, caplist, param, val)
    else:
        prepare_typed_capability(cont, caplist, param, val)
    # Add default capabilities exchange registers and thread control

def create_default_capabilities(cont):
    caplist = cont.caplist["PAGER"]
    for captype in ['tctrl', 'exregs']:
        # Set the string
        caplist.caps[captype] = cap_strings[captype]
        # Replace substitues with values
        templ = Template(caplist.caps[captype])
        caplist.caps[captype] = templ.safe_substitute(cid = cont.id)

