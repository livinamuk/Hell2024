#!/usr/bin/env python3
#
# Copyright (c) 2020-2022 LunarG, Inc.
# Copyright (c) 2022 Valve Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#
# Utility for invoking gfxrecon commands
# Usage:
#
#     gfxrecon.py [capture|compress|convert|extract|info|optimize|replay] [<args>]
#
#         args is a command-specific argument list

import argparse
import os
import sys
import subprocess
import platform

argv = sys.argv
argc = len(sys.argv)

# Supported commands
valid_commands = [
    'capture-vulkan',
    'compress',
    'convert',
    'extract',
    'info',
    'optimize',
    'replay'
]

deprecated_commands = [
    'capture'  # alias for `capture-vulkan`
]

def IsWindows():
   return (sys.platform == 'win32' or sys.platform == 'cygwin')

# Function to return an executable for the given command.
# Searches for an executable command in:
#    current directory
#    PATH
#    ../cmd  (on Linux only, so that this script works when run from a repo build)
#    ../cmd/*  (on Windows only, so that this script works when run from a repo build)
# If the above search fails, then it tries to find a python script named cmd.py in:
#    current directory
#    PATH
#    ../cmd
# If the a python script is found, the availability of a python interpreter is checked.
#
# On success, returns a list that can be passed to subprocess.run to execute the command,
# possibly including the python interpreter as the first element in the list.
#
# If an executable/python script is not found, or a python interpreter for a python script is
# not found, prints an error message and exits.

def GetExecutable(cmd):

    def IsExe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    # Set the command we are looking for
    if IsWindows():
        cmdexe = 'gfxrecon-' + cmd + '.exe'
    else:
        cmdexe = 'gfxrecon-' + cmd

    # Search for cmdexe in current dir
    if IsExe(cmdexe):
        return [os.path.join('.', cmdexe)]

    # Search for cmdexe in PATH
    for path in os.environ['PATH'].split(os.pathsep):
        c = os.path.join(path, cmdexe)
        if IsExe(c):
            return [c]

    # Windows: Search for cmdexe in <scriptdir>/../cmd/*
    # (<scriptdir> is the dir that this script is located in.)
    scriptdir = os.path.dirname(os.path.realpath(__file__))
    if IsWindows():
       for buildtype in ['Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel']:
            c = os.path.join(scriptdir, '..', cmd, buildtype, cmdexe)
            if IsExe(c):
                return [c]
    else:
        # Linux: Search for cmdexe in <scriptdir>/../cmd
        c = os.path.join(scriptdir, '..', cmd, cmdexe)
        if IsExe(c):
            return [c]

    cmdpy = 'gfxrecon-' + cmd + '.py'

    # Search for cmdpy in current dir
    if os.path.isfile(cmdpy):
        return [sys.executable, cmdpy]

    # Search for cmdpy in PATH
    for path in os.environ['PATH'].split(os.pathsep):
        c=os.path.join(path, cmdpy)
        if os.path.isfile(c):
            return [sys.executable, c]

    # Search for cmdpy <scriptdir>/../cmd
    c = os.path.join(scriptdir, '..', cmd, cmdpy)
    if os.path.isfile(c):
        return [sys.executable, c]

    # Didn't find the executable or py command, error out
    print('Error: Cannot find ' + cmdexe + ' or ' + cmdpy + ' to execute')
    sys.exit(1)

def CreateCommandParser():
    parser = argparse.ArgumentParser(description='GFXReconstruct utility launcher.')
    parser.add_argument(
        'command',
        choices=(valid_commands + deprecated_commands),
        type=str.lower,
        metavar='command',
        help='Command to execute. Valid options are [{}]'.format(
            ', '.join(valid_commands)))
    parser.add_argument('args', nargs=argparse.REMAINDER, help='Command-specific argument list. Specify -h after command name for command help.')
    return parser

if __name__ == '__main__':

    # We don't support running under Cygwin Python
    if sys.platform == 'cygwin':
        print(os.path.basename(__file__) + ' error: Cygwin Python not supported')
        sys.exit(1)

    command_parser = CreateCommandParser()
    command = command_parser.parse_args()

    # Fixup and warn for any deprecated commands.
    if command.command == 'capture':
        print(
            "Warning: the 'capture' command is deprecated and will be removed in a future update. Use 'capture-vulkan' instead.",
            flush=True)
        command.command = 'capture-vulkan'

    extras = sys.argv[2::]
    cmd = GetExecutable(command.command)
    result = subprocess.run(cmd + extras)
    sys.exit(result.returncode)
