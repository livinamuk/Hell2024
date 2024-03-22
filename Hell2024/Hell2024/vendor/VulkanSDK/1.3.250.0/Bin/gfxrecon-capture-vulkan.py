#!/usr/bin/env python3
#
# Copyright (c) 2020 LunarG, Inc.
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
# Utility for capturing Vulkan API calls using gfxreconstruct

'''Capture a trace of a Vulkan binary.
'''

import argparse
import os
import platform
import shutil
import sys
import subprocess


def usage_message():
    '''Output the script usage message

    We use a custom usage message because Python's arg parser doesn't provide
    a very good usage message for positional arguments.
    (The default usage message for positional args is as an ellipse. We would
    rather have "<program> [<programArgs>]".)

    :return: A well formatted usage message string
    '''
    message = [
        'usage gfxrecon-capture-vulkan.py [-h]',
        '                                 [-w dir]',
        '                                 [-o capture_file]',
        '                                 [-f capture_frames]',
        '                                 [--no-file-timestamp]',
        '                                 [--trigger {F1-F12,TAB,CTRL}]',
        '                                 [--trigger-frames frame_count]',
        '                                 [--compression-type {LZ4,ZLIB,ZSTD,NONE}]',
        '                                 [--file-flush]',
        '                                 [--log-level {debug,info,warn,error,fatal}]',
        '                                 [--log-file <file>]',
        '                                 [--memory-tracking-mode {page_guard,assisted,unassisted}]',
        '                                 [--capture-layer <capture_layer_path>',
    ]
    if sys.platform == 'win32':
        message.append('                          [--log-debugview]')
    message.append('                          <program> [<program_args>]')
    return '\n'.join(message)


class SmartFormatter(argparse.HelpFormatter):
    '''Used by the argument parser to assist in breaking argument help text
    into multiple lines.

    It is used whenever the help text starts with 'R|'.
    '''

    def _split_lines(self, text, width):
        '''Split the argument help into multiple lines

        In addition to the default behavior it will also break lines that start
        with R|.
        '''
        if text.startswith('R|'):
            return text[2:].splitlines()
        return argparse.HelpFormatter._split_lines(self, text, width)


def print_error_and_exit(msg):
    '''Print error message and exit with non-zero status
    '''
    print(os.path.basename(__file__) + ' error: ' + msg)
    sys.exit(1)


def set_env_var(name, value):
    '''Set an environment variable to a given value or remove it from the
    environment if None
    '''
    if value is not None:
        os.environ[name] = value
    elif name in os.environ:
        del os.environ[name]


def create_argument_parser():
    '''Create the argument parser used to parse command line arguments

    Create an argument parser with a custom description and useage.
    Add command line arguments and options to the parser.
    '''
    TRIGGER_KEY_CHOICES = ['F1', 'F2', 'F3', 'F4', 'F5', 'F6',
                           'F7', 'F8', 'F9', 'F10', 'F11', 'F12', 'TAB', 'CTRL']
    COMPRESSION_CHOICES = ['LZ4', 'ZLIB', 'ZSTD', 'NONE']
    LOG_LEVEL_CHOICES = ['debug', 'info', 'warn', 'error', 'fatal']
    MEMORY_TRACKING_MODE_CHOICES = ['page_guard', 'assisted', 'unassisted']

    parser = argparse.ArgumentParser(
        prog=os.path.basename(sys.argv[0]),
        description=__doc__,
        usage=usage_message(),
        allow_abbrev=False,
        formatter_class=argparse.RawTextHelpFormatter)

    # Common optional args
    # All arguments default to None, indicating they should be unset in the capture environment
    parser.add_argument(
        '-w', '--working-dir', dest='working_dir', metavar='<dir>',
        help='Set CWD to this directory before running the program')
    parser.add_argument(
        '-o', '--capture-file', dest='capture_file', metavar='<capture_file>',
        default='gfxrecon_capture.gfxr',
        help='Name of the capture file, default is gfxrecon_capture.gfxr')
    parser.add_argument(
        '-f', '--capture-frames', dest='capture_frames',
        metavar='<capture_frames>',
        help='List of frames to capture, default is all frames')
    parser.add_argument(
        '--no-file-timestamp', dest='file_timestamp', action='store_const', const='false',
        help='Do not add a timestamp to the capture file name')
    parser.add_argument(
        '--trigger', dest='trigger', choices=TRIGGER_KEY_CHOICES,
        help='Specify a hotkey to start/stop capture')
    parser.add_argument('--trigger-frames', dest='trigger_frames', metavar='<frame_count>',
        help='Specify a limit on the number of frames captured via hotkey')
    parser.add_argument(
        '--compression-type', dest='compression_type', choices=COMPRESSION_CHOICES,
        help='Specify the type of compression to use in the capture file, default is LZ4')
    parser.add_argument(
        '--file-flush', dest='file_flush', action='store_const', const='true',
        help='Flush output stream after each packet is written to capture file')
    parser.add_argument(
        '--log-level', dest='log_level', choices=LOG_LEVEL_CHOICES,
        help='Specify highest level message to log, default is info')
    parser.add_argument(
        '--log-file', dest='log_file', metavar='<log_file>',
        help='Write log messages to a file at the specified path. Default is: Empty string (file logging disabled)')
    parser.add_argument(
        '--log-debugview', dest='log_debug_view', action='store_const', const='true',
        help='Log messages with OutputDebugStringA' if sys.platform == 'win32' else argparse.SUPPRESS)
    parser.add_argument(
        '--memory-tracking-mode', dest='memory_tracking_mode', choices=MEMORY_TRACKING_MODE_CHOICES,
        help='\n'.join([
            'R|Method used to track changes to memory mapped objects:',
            '  - page_guard: use pageguard to track changes (default)',
            '  - assisted: application will call vkFlushMappedMemoryRanges',
            '  - for memory to be written to the capture file',
            '  - unassisted: all mapped memory will be written to the',
            '  - capture file during VkQueueSubmit and VkUnmapMemory']))
    parser.add_argument(
        '--capture-layer', dest='capture_layer', metavar='<capture_layer>',
        default=None,
        help='\n'.join([
            'The path to the capture layer.',
            '',
            'The path specified must contain both the layer JSON, and the',
            'layer library',
            'It is recommended to use an absolute path for this option.']))

    # Required args
    parser.add_argument(
        'program_and_args', metavar='<program> [<program args>]', nargs=argparse.REMAINDER,
        help='Program to capture, optionally followed by program arguments')

    return parser


def print_args(args):
    '''Print command line argument values.

    Used for debug.
    '''
    print('working-dir', args.working_dir)
    print('capture-file', os.path.abspath(args.capture_file))
    print('capture-frames', args.capture_frames)
    print('no-file-timestamp', args.file_timestamp)
    print('trigger', args.trigger)
    print('trigger-frames', args.trigger_frames)
    print('compression-type', args.compression_type)
    print('file-flush', args.file_flush)
    print('log-level', args.log_level)
    print('log-file', args.log_file)
    print('log-debugview', args.log_debug_view)
    print('memory-tracking-mode', args.memory_tracking_mode)
    print('program_and_args', args.program_and_args)


def get_command_path(args):
    '''Get the full path to the command to execute
    '''
    # Replace ~ or ~user with the user's home path before calling shutil.which()
    programName = os.path.expanduser(args.program_and_args[0])
    return shutil.which(programName)


def validate_args(args):
    '''Validate command line arguments
    '''
    # Verify working_dir exists and is a directory.
    if args.working_dir is not None:
        if (not os.path.isdir(args.working_dir)):
            print_error_and_exit('Working directory ' +
                                 args.working_dir + ' does not exist')

    # Make sure program was specified. arg parser doesn't allow specifying program_and_args
    # as required.
    if len(args.program_and_args) == 0:
        print('usage: ' + usage_message())
        print_error_and_exit('<program> must be specified')

    # Verify programName exists and is executable.
    if get_command_path(args) is None:
        print_error_and_exit('Cannot find program ' +
                             programName + ' to execute')

    # Verify capture_file directory exists and is a valid directory.
    capture_dir = os.path.dirname(os.path.abspath(args.capture_file))
    if (not os.path.exists(capture_dir)):
        print_error_and_exit('Capture file output directory ' +
                             capture_dir + ' does not exist')
    if (not os.path.isdir(capture_dir)):
        print_error_and_exit('Capture file output directory ' +
                             capture_dir + ' is not a valid directory')

    # Verify the capture layer paht is a valid directory
    if (args.capture_layer is not None) and (not os.path.isdir(args.capture_layer)):
        print_error_and_exit('Capture layer path is not a directory')


def set_env_vars(args):
    '''Set environment variables for capture layer
    '''
    # Set VK_INSTANCE_LAYERS
    # If gfxr layer is not already in VK_INSTANCE_LAYER, append gfxr layer
    # to VK_INSTANCE_LAYERS
    if os.getenv('VK_INSTANCE_LAYERS') is None:
        os.environ['VK_INSTANCE_LAYERS'] = 'VK_LAYER_LUNARG_gfxreconstruct'
    elif (not ('VK_LAYER_LUNARG_gfxreconstruct' in os.getenv('VK_INSTANCE_LAYERS'))):
        os.environ['VK_INSTANCE_LAYERS'] = os.environ['VK_INSTANCE_LAYERS'] + \
            os.pathsep + 'VK_LAYER_LUNARG_gfxreconstruct'
    if args.capture_layer is not None:
        # Prefix the layer path provided by the user to the layer search path
        path_delimiter = ':'
        if 'windows' == platform.system().lower():
            path_delimiter = ';'
        vk_layer_path = ''
        if 'VK_LAYER_PATH' in os.environ:
            vk_layer_path = os.environ['VK_LAYER_PATH']
        os.environ['VK_LAYER_PATH'] = path_delimiter.join([
            args.capture_layer, vk_layer_path])

    # Set GFXRECON_* capture options
    # The capture layer will validate these options and generate errors as needed
    set_env_var('GFXRECON_CAPTURE_FILE', os.path.abspath(args.capture_file))
    set_env_var('GFXRECON_CAPTURE_FRAMES', args.capture_frames)
    set_env_var('GFXRECON_CAPTURE_FILE_TIMESTAMP', args.file_timestamp)
    set_env_var('GFXRECON_CAPTURE_TRIGGER', args.trigger)
    set_env_var('GFXRECON_CAPTURE_TRIGGER_FRAMES', args.trigger_frames)
    set_env_var('GFXRECON_CAPTURE_COMPRESSION_TYPE', args.compression_type)
    set_env_var('GFXRECON_CAPTURE_FILE_FLUSH', args.file_flush)
    set_env_var('GFXRECON_LOG_LEVEL', args.log_level)
    set_env_var('GFXRECON_LOG_FILE', args.log_file)
    set_env_var('GFXRECON_LOG_OUTPUT_TO_OS_DEBUG_STRING', args.log_debug_view)
    set_env_var('GFXRECON_MEMORY_TRACKING_MODE', args.memory_tracking_mode)


def print_env_var(env_var):
    '''Print the given envrionment variable

    Used for debug.
    :param env_var: The envrionment variable printed
    '''
    if env_var in os.environ:
        print(env_var, os.environ[env_var])
    else:
        print(env_var, 'None')


def PrintLayerEnv():
    '''Print all GFXReconstruct layer environment variables

    Used for debug.
    '''
    print_env_var('GFXRECON_CAPTURE_COMPRESSION_TYPE')
    print_env_var('GFXRECON_CAPTURE_FILE')
    print_env_var('GFXRECON_CAPTURE_FILE_FLUSH')
    print_env_var('GFXRECON_CAPTURE_FILE_TIMESTAMP')
    print_env_var('GFXRECON_CAPTURE_FRAMES')
    print_env_var('GFXRECON_CAPTURE_TRIGGER')
    print_env_var('GFXRECON_LOG_FILE')
    print_env_var('GFXRECON_LOG_LEVEL')
    print_env_var('GFXRECON_LOG_OUTPUT_TO_OS_DEBUG_STRING')
    print_env_var('GFXRECON_MEMORY_TRACKING_MODE')
    print_env_var('VK_INSTANCE_LAYERS')
    print_env_var('VK_LAYER_PATH')


if '__main__' == __name__:

    # We don't support running under Cygwin Python
    if sys.platform == 'cygwin':
        print_error_and_exit("Cygwin Python not supported")

    # Get and validate args
    parser = create_argument_parser()
    args = parser.parse_args()
    if 'debug' == args.log_level:
        print_args(args)   # For debugging
    validate_args(args)

    # Set up environment
    set_env_vars(args)
    if 'debug' == args.log_level:
        PrintLayerEnv()    # For debugging

    # If working_dir was specified, make it the cwd
    if args.working_dir is not None:
        os.chdir(args.working_dir)

    # Run the program and and exit with the exit status of the program
    print('Executing program', get_command_path(args))
    result = subprocess.run(args.program_and_args, capture_output=True)
    if 0 != result.returncode:
        print('Errors:\n', result.stderr.decode('utf-8'))
    print('Output:\n', result.stdout.decode('utf-8'))
    sys.exit(result.returncode)
