#!/usr/bin/env python3
#
# Copyright (c) 2023 LunarG, Inc.
# Copyright (c) 2023 Valve Corporation
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
# Stub for redirecting to new gfxrecon-capture-vulkan command
# Usage:
#
#     gfxrecon-capture.py [args]
#
#         args is a command-specific argument list

import argparse
import os
import sys
import subprocess
import platform

argv = sys.argv
argc = len(sys.argv)

if __name__ == '__main__':

    # We don't support running under Cygwin Python
    if sys.platform == 'cygwin':
        print(os.path.basename(__file__) + ' error: Cygwin Python not supported')
        sys.exit(1)

    print("Warning: the 'gfxrecon-capture.py' script is deprecated and will be removed in a future update. Use 'gfxrecon-capture-vulkan.py' instead.", flush=True)
    result = subprocess.run(["gfxrecon-capture-vulkan.py",] + argv[1:], shell = True)
    sys.exit(result.returncode)
