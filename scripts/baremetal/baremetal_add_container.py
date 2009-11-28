#! /usr/bin/env python2.6
# -*- mode: python; coding: utf-8; -*-
#
# Script to add/remove project to baremetal
# menu of main screen
#
# This script should be called from project root directory
#
import os, sys, shutil, re
from optparse import OptionParser
from os.path import join
from shutil import copytree

def parse_cmdline_options():
    usage = "usage: %prog [options] arg"
    parser = OptionParser(usage)

    parser.add_option("-a", "--add", action = "store_true", default = False,
            dest = "addproject", help = "Add new project to baremetal projects")
    parser.add_option("-d", "--del", action = "store_true", default = False,
            dest = "delproject", help = "Delete existing project from baremetal projects")
    parser.add_option("--desc", type = "string", dest = "projdesc",
            help = "Description to be used for newly added project")
    parser.add_option("--id", type = "int", dest = "srcid",
            help = "If used with -a, Baremetal container id, to be used as source for source project\
                                 If used with -d, Baremetal container id to be deleted")

    (options, args) = parser.parse_args()

    # Sanity checks
    if (not options.addproject and not options.delproject) or \
            (options.addproject and options.delproject):
        parser.error("Only one of -a or -d needed, use -h argument for help")
        exit()

    if options.addproject:
        add_del = 1
        if not options.projdesc or not options.srcid:
            parser.error("--desc or --id missing, use -h argument for help")
            exit()

    if options.delproject:
        add_del = 0
        if options.projdesc or not options.srcid:
            parser.error("--desc provided or --id missing with -d, use -h argument for help")
            exit()

    return options.projdesc, options.srcid, add_del

def container_cml_templ_del_symbl(srcid):
    cont_templ = "config/cml/container_ruleset.template"
    sym = "CONT%(cn)d_BAREMETAL_PROJ" + str(srcid)

    buffer = ""
    with open(cont_templ, 'r') as fin:
        exist = False
        # Prepare buffer for new cont_templ with new project symbols added
        for line in fin:
            parts = line.split()

            # Find out where baremetal symbols start in cont_templ
            if len(parts) > 1 and parts[0] == sym:
                exist = True
                continue
            elif len(parts) == 1 and parts[0] == sym:
                continue

            buffer += line
        if exist == False:
            print "Project named baremetal" + str(srcid) + " does not exist"
            exit()

    # Write new cont_templ
    with open(cont_templ, 'w+') as fout:
        fout.write(buffer)


def container_cml_templ_add_symbl(projdesc):
    cont_templ = "config/cml/container_ruleset.template"

    pattern = re.compile("(CONT\%\(cn\)d_BAREMETAL_PROJ)([0-9]*)")
    new_sym = "CONT%(cn)d_BAREMETAL_PROJ"

    # ID for new project
    projid = -1

    buffer = ""
    with open(cont_templ, 'r') as fin:
        baremetal_sym_found = False

        # Prepare buffer for new cont_templ with new project symbols added
        for line in fin:
            parts = line.split()

            # Find out where baremetal symbols start in cont_templ
            if len(parts) > 1 and re.match(pattern, parts[0]):
                baremetal_sym_found = True
                # Last project id in use for baremetal conts
                projid = \
                    int(parts[0][len("CONT%(cn)d_BAREMETAL_PROJ"):])

            # We are done with baremetal symbols, add new symbol to buffer
            elif baremetal_sym_found == True:
                baremetal_sym_found = False
                sym_def = new_sym + str(int(projid) + 1) + \
                        "\t\'" + projdesc + "\'\n"
                buffer += sym_def

            # Search for baremetal menu and add new project symbol
            elif len(parts) == 1 and parts[0] == new_sym + str(projid):
                sym_reference = "\t" + new_sym + str(projid + 1) + "\n"
                line += sym_reference

            buffer += line

    # Write new cont_templ
    with open(cont_templ, 'w+') as fout:
        fout.write(buffer)

    return projid + 1

def add_project(projdesc, srcid):
    dest_id = container_cml_templ_add_symbl(projdesc)

    baremetal_dir = "conts/baremetal"
    src_dir = join(baremetal_dir, "baremetal" + str(srcid))
    dest_dir = join(baremetal_dir, "baremetal" + str(dest_id))

    print "Copying source files from " + src_dir + " to " + dest_dir
    shutil.copytree(src_dir, dest_dir)
    print "Done, New project baremetal" +str(dest_id) + \
          " is ready to be used."

def del_project(srcid):
    container_cml_templ_del_symbl(srcid)

    baremetal_dir = "conts/baremetal"
    src_dir = join(baremetal_dir, "baremetal" + str(srcid))

    print "Deleting source files from " + src_dir
    shutil.rmtree(src_dir, "ignore_errors")
    print "Done.."

def main():
    projdesc, srcid, add_del = parse_cmdline_options()

    if add_del == 1:
        add_project(projdesc, srcid)
    else:
        del_project(srcid)


if __name__ == "__main__":
    main()

