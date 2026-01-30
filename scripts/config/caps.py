#! /usr/bin/env python3
# -*- mode: python; coding: utf-8; -*-
from .projpaths import *
from string import Template

cap_strings = {
    "ipc": """
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_IPC | ${target_rtype},
\t\t\t\t.access = CAP_IPC_SEND | CAP_IPC_RECV
\t\t\t\t          | CAP_IPC_FULL | CAP_IPC_SHORT
\t\t\t\t          | CAP_IPC_EXTENDED | CAP_CHANGEABLE
\t\t\t\t          | CAP_REPLICABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
""",
    "tctrl": """
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
""",
    "irqctrl": """
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_IRQCTRL | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_IRQCTRL_REGISTER | CAP_IRQCTRL_WAIT
\t\t\t\t          | CAP_CHANGEABLE | CAP_REPLICABLE
\t\t\t\t          | CAP_TRANSFERABLE,
\t\t\t\t.start = IRQ_RANGE_START, .end = IRQ_RANGE_END, .size = 0,
\t\t\t},
""",
    "exregs": """
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_EXREGS | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_EXREGS_RW_PAGER
\t\t\t\t          | CAP_EXREGS_RW_UTCB | CAP_EXREGS_RW_SP
\t\t\t\t          | CAP_EXREGS_RW_PC | CAP_EXREGS_RW_REGS
\t\t\t\t          | CAP_CHANGEABLE | CAP_REPLICABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
""",
    "threadpool": """
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY
\t\t\t\t	  | CAP_RTYPE_THREADPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t.size = ${size},
\t\t\t},
""",
    "spacepool": """
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_SPACEPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t.size = ${size},
\t\t\t},
""",
    "mutexpool": """
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_MUTEXPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t.size = ${size},
\t\t\t},
""",
    "mappool": """
\t\t\t[${idx}] = {
\t\t\t\t/* For pmd accounting */
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_MAPPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t/* Function of mem regions, nthreads etc. */
\t\t\t\t.size = ${size},
\t\t\t},
""",
    "capctrl": """
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_CAP | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_CAP_GRANT | CAP_CAP_READ
\t\t\t\t          | CAP_CAP_SHARE | CAP_CAP_REPLICATE
\t\t\t\t          | CAP_CHANGEABLE | CAP_REPLICABLE
\t\t\t\t          | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
""",
    "umutex": """
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_UMUTEX | CAP_RTYPE_CONTAINER,
\t\t\t\t.access = CAP_UMUTEX_LOCK | CAP_UMUTEX_UNLOCK
\t\t\t\t          | CAP_CHANGEABLE | CAP_REPLICABLE
\t\t\t\t          | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0, .size = 0,
\t\t\t},
""",
    "cappool": """
\t\t\t[${idx}] = {
\t\t\t\t.target = ${cid},
\t\t\t\t.type = CAP_TYPE_QUANTITY | CAP_RTYPE_CAPPOOL,
\t\t\t\t.access = CAP_CHANGEABLE | CAP_TRANSFERABLE,
\t\t\t\t.start = 0, .end = 0,
\t\t\t\t.size = ${size},
\t\t\t},
""",
}


# Target type to rtype mapping
TARGET_RTYPE_MAP = {
    "CURRENT_CONTAINER": ("CAP_RTYPE_CONTAINER", True),
    "CURRENT_PAGER_SPACE": ("CAP_RTYPE_SPACE", True),
    "OTHER_CONTAINER": ("CAP_RTYPE_CONTAINER", False),
    "OTHER_PAGER": ("CAP_RTYPE_THREAD", False),
}


def _apply_target_type(caplist, capkey, ttype, cont):
    """Apply target type substitution to a capability template."""
    templ = Template(caplist.caps[capkey])
    if ttype not in TARGET_RTYPE_MAP:
        return
    rtype, use_cont_id = TARGET_RTYPE_MAP[ttype]
    if use_cont_id:
        caplist.caps[capkey] = templ.safe_substitute(target_rtype=rtype, cid=cont.id)
    else:
        caplist.caps[capkey] = templ.safe_substitute(target_rtype=rtype)


def prepare_custom_capability(cont, caplist, param, val):
    if "TYPE" in param:
        capkey, captype, _ = param.split("_", 2)
        caplist.caps[capkey.lower()] = cap_strings[captype.lower()]
    elif "TARGET" in param:
        target_parts = param.split("_", 2)
        capkey = target_parts[0].lower()
        templ = Template(caplist.caps[capkey])
        if len(target_parts) == 2:
            caplist.caps[capkey] = templ.safe_substitute(cid=val)
        elif len(target_parts) == 3:
            _apply_target_type(caplist, capkey, target_parts[2], cont)


def _init_capability(caplist, captype, cont):
    """Initialize a capability with default container id for pools/irqctrl."""
    caplist.caps[captype] = cap_strings[captype]
    if captype.endswith("pool") or captype.endswith("irqctrl"):
        templ = Template(caplist.caps[captype])
        caplist.caps[captype] = templ.safe_substitute(cid=cont.id)


def prepare_typed_capability(cont, caplist, param, val):
    captype, params = param.split("_", 1)
    captype = captype.lower()

    # Skip unknown capability types (e.g., "device" mappings are not kernel caps)
    if captype not in cap_strings:
        return

    # Auto-initialize capability if not yet seen (handles out-of-order symbols)
    if captype not in caplist.caps:
        _init_capability(caplist, captype, cont)

    # USE makes us assign the initial cap string with blank fields
    if "USE" in params:
        if captype not in caplist.caps:
            _init_capability(caplist, captype, cont)

    # Fill in the blank size field
    elif "SIZE" in params:
        templ = Template(caplist.caps[captype])
        caplist.caps[captype] = templ.safe_substitute(size=val)

    # Fill in capability target type and target id fields
    elif "TARGET" in params:
        target_parts = params.split("_", 1)
        if len(target_parts) == 2:
            _apply_target_type(caplist, captype, target_parts[1], cont)
        else:
            templ = Template(caplist.caps[captype])
            caplist.caps[captype] = templ.safe_substitute(cid=val)


def prepare_capability(cont, cap_owner, param, val):
    caplist = cont.caplist[cap_owner]  # Passing either pager or container caplist.
    if "CUSTOM" in param:
        prepare_custom_capability(cont, caplist, param, val)
    else:
        prepare_typed_capability(cont, caplist, param, val)
    # Add default capabilities exchange registers and thread control


def create_default_capabilities(cont):
    caplist = cont.caplist["PAGER"]
    for captype in ["tctrl", "exregs"]:
        # Set the string
        caplist.caps[captype] = cap_strings[captype]
        # Replace substitues with values
        templ = Template(caplist.caps[captype])
        caplist.caps[captype] = templ.safe_substitute(cid=cont.id)
