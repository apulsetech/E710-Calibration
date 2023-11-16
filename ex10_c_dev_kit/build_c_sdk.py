#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2022 - 2023 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
Build the C SDK sources
Base class to use for Ex10 app testing
"""
# pylint: disable=locally-disabled, consider-using-f-string

from __future__ import (
    division, absolute_import, print_function, unicode_literals)

import sys
import os
import re
import subprocess
import shutil
import argparse
import multiprocessing

# This script must be run from the directory ex10_c_dev_kit.
_DEFAULT_BOARD_TARGET  = 'e710_ref_design'
_DEFAULT_TOOLCHAIN     = os.path.join('board',
                                      _DEFAULT_BOARD_TARGET,
                                      'Toolchain-gcc-linux.cmake')
_DEFAULT_BUILD_TARGETS = ['libs', 'examples']
_DEFAULT_BUILD_TYPE    = 'Release'
_DEFAULT_BUILD_JOBS    = multiprocessing.cpu_count()
_DEFAULT_VERBOSITY     = False

# CMake versions prior to version 3.16.0 have trouble parsing multiple
# build targets on the command line.
# i.e. cmake --build --targets <target_1> <target_2> fails to build.
# So the target list has to build one-at-a-time.
_CMAKE_VERSION_ROBUST = (3, 16, 0)


def _command_string(command_arg_list):
    """ From a list of command arguments return a string. """
    command = ''
    for arg in command_arg_list:
        if not isinstance(arg, list):
            command += arg + ' '
        else:
            for arg_in_list in arg:
                command += arg_in_list + ' '
    return command.strip()


class Ex10SdkBuilder():
    # pylint: disable=locally-disabled, too-many-instance-attributes
    """
    A class for wrapping the cmake build steps in the C SDK
    :param c_sdk_root_path: The root directory of the C SDK; i.e. ex10_c_dev_kit
    :param build_root_path: The artifact output directory.
                            Defaults to 'c_sdk_root_path/build'
    :param toolchain_file:  The CMake toolchain file.
    :param board_target:    The board type.
    :param build_type:      Choices: 'Release', 'Debug', 'Coverage', 'Profiling'
    :param compile_definitions:
        A dictionary of compiler definitions. Each node in this parameter is
        defined as a symbol during compilation.
        Example: {'LTTNG': 1} will set the LTTNG symbol to a 1 when compiling.
    :param build_jobs:      The number of processes to use when building.
    :param verbose:         True: print more information, False print normally.
    """

    def __init__(self,
                 c_sdk_root_path=None,
                 build_root_path=None,
                 toolchain_file=_DEFAULT_TOOLCHAIN,
                 board_target=_DEFAULT_BOARD_TARGET,
                 build_targets=None,
                 build_type=_DEFAULT_BUILD_TYPE,
                 cmd_line_definitions=None,
                 compiler_definitions=None,
                 build_jobs=_DEFAULT_BUILD_JOBS,
                 verbose=_DEFAULT_VERBOSITY):
        # pylint: disable=locally-disabled, too-many-arguments
        if c_sdk_root_path is None:
            # The default is that this script can be run from either
            # ex10_dev_kit or ex10_c_dev_kit directory, provided that they
            # installed at the same level in the directory structure.
            c_sdk_root = os.path.join(os.getcwd(), '..', 'ex10_c_dev_kit')
            self.c_sdk_root_path = os.path.normpath(c_sdk_root)
        else:
            self.c_sdk_root_path = c_sdk_root_path

        if build_root_path is None:
            self.build_root_path = os.path.join(self.c_sdk_root_path, 'build')
        else:
            self.build_root_path = build_root_path

        py_sdk_root = os.path.join(self.c_sdk_root_path, '..', 'ex10_dev_kit')
        self.py_sdk_root_path = os.path.normpath(py_sdk_root)

        # Where the lib_py2c.so will be copied, once built.
        self.lib_py2c_install_path = os.path.join(self.py_sdk_root_path,
                                                  'py2c_interface')
        # Where the artifacts will be built.
        self.build_path = os.path.join(self.c_sdk_root_path, 'build')
        # The compiler tool chain path/file.
        self.toolchain = os.path.join(self.c_sdk_root_path, toolchain_file)

        self.board_target = board_target
        self.build_targets = build_targets
        self.build_type = build_type
        self.cmd_line_definitions = (
            {} if cmd_line_definitions is None else cmd_line_definitions)
        self.compiler_definitions = (
            {} if compiler_definitions is None else compiler_definitions)

        self.build_jobs = build_jobs
        self.verbose = verbose
        self._cmake_version_string = None
        self._cmake_version_tuple = None

        # Capture the subprocess call return code of the cmake subprocess call.
        self._cmake_return_code = 0

    @property
    def cmake_version_string(self):
        """ Get the CMake version as a string """
        self.get_cmake_version()
        return self._cmake_version_string

    @property
    def cmake_version_tuple(self):
        """ Get the CMake version as a tuple """
        self.get_cmake_version()
        return self._cmake_version_tuple

    @property
    def cmake_return_code(self):
        """ Get the CMake execution return code """
        return self._cmake_return_code

    def _run_command(self, command, capture_output=False):
        """ Run the command, block until complete """
        # Unfortunately, python2 subprocess.Popen() does not support 'with':
        # pylint: disable=locally-disabled, consider-using-with
        self._cmake_return_code = 0
        if capture_output:
            build_process = subprocess.Popen(command,
                                             stdout=subprocess.PIPE,
                                             stderr=subprocess.PIPE)
        else:
            build_process = subprocess.Popen(command, stderr=subprocess.STDOUT)

        # Wait for cmake build generation to complete.
        while build_process.poll() is None:
            pass

        self._cmake_return_code = build_process.returncode
        if build_process.returncode != 0:
            raise RuntimeError("C SDK build command '{}' failed: {}".
                               format(_command_string(command),
                                      self.cmake_return_code))

        # Note: if capture_output is false, then stdout, stderr will be None.
        stdout, stderr = build_process.communicate()
        return stdout, stderr

    def get_cmake_version(self):
        """
        Run the command 'cmake --version', extract and return the 'X.Y.Z'
        version string and return it.
        """
        if self._cmake_version_string is None or self._cmake_version_tuple is None:
            cmake_version_command = ['cmake', '--version']
            cmake_version_response = self._run_command(cmake_version_command,
                                                       capture_output=True)[0]
            cmake_version_match = re.search(r'[0-9]+\.[0-9]+\.[0-9]+',
                                            cmake_version_response.decode('utf-8'))
            self._cmake_version_string = cmake_version_match.group(0)
            v_strings = self._cmake_version_string.split('.')
            self._cmake_version_tuple = (
                int(v_strings[0]), int(v_strings[1]), int(v_strings[2]))
        return self._cmake_version_tuple

    def configure(self):
        """ Configure the build using CMake """
        # NOTE: For reasons unknown, do not put a space between -D <argument>
        # or CMake will ignore it when run from python.
        cmake_generate_command = [
            'cmake',
            '-DCMAKE_TOOLCHAIN_FILE={}'.format(self.toolchain),
            '-DBOARD_TARGET={}'.format(self.board_target),
            '-DCMAKE_BUILD_TYPE={}'.format(self.build_type)]

        if self.verbose:
            cmake_generate_command.append('--log-level=debug')

        for key, value in self.cmd_line_definitions.items():
            cmake_generate_command.append('-D{}={}'.format(key, value))
        for key, value in self.compiler_definitions.items():
            cmake_generate_command.append('-D{}={}'.format(key, value))

        cmake_generate_command.extend(['-S', self.c_sdk_root_path,
                                       '-B', self.build_root_path])

        print('------------------------- Build Generation:')
        print(_command_string(cmake_generate_command))
        self._run_command(cmake_generate_command)

    def build(self):
        """ Build the C SDK """
        cmake_build_command = ['cmake', '--build', self.build_root_path]
        if self.verbose:
            cmake_build_command.append('--verbose')
        if self.build_jobs not in [0, 1]:
            cmake_build_command.append('--parallel')
            cmake_build_command.append('{}'.format(self.build_jobs))
        if self.build_targets:
            cmake_build_command.append('--target')
            if self.cmake_version_tuple < _CMAKE_VERSION_ROBUST:
                for target in self.build_targets:
                    build_command = cmake_build_command[:]
                    build_command.append(target)
                    print('------------------------- Building target {}:'
                          .format(target))
                    print(_command_string(build_command))
                    self._run_command(build_command)
                return
            else:
                cmake_build_command.extend(self.build_targets)
        print('------------------------- Building:')
        print(_command_string(cmake_build_command))
        self._run_command(cmake_build_command)

    def install_py2c(self):
        """
        Install the lib_py2c.so into
        """
        built_file = os.path.join(
            self.build_root_path, self.board_target, 'lib_py2c.so')
        install_path = os.path.join(
            self.c_sdk_root_path, '..', 'ex10_dev_kit', 'py2c_interface')
        install_path = os.path.normpath(install_path)
        install_file_path = os.path.join(install_path, 'lib_py2c.so')
        if self.build_type == 'Coverage':
            install_file_path = os.path.join(install_path,
                                            'lib_py2c-coverage.so')
        print('------------------------- Installing:')
        print('copying {} -> {}'.format(built_file, install_file_path))
        shutil.copyfile(built_file, install_file_path)

    def scrub(self):
        """ Remove the artifact build directory completely """
        print('------------------------- Scrubbing:')
        print('removing build directory {}'.format(self.build_root_path))
        shutil.rmtree(self.build_root_path, ignore_errors=True)


def main():
    """ Command line interface to the Ex10SdkBuilder class """

    description = 'Build the C SDK'

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument('--c-sdk-root',
                        default=None,
                        help="locate the C SDK 'ex10_c_sdk' directory"
                        ", default = current working directory")

    parser.add_argument('--build-path',
                        default=None,
                        help='set the artifact output directory'
                        ', default = ex10_c_sdk/build')

    parser.add_argument('--toolchain',
                        default=_DEFAULT_TOOLCHAIN,
                        help='set the cmake toolchain file '
                        ', default = {}'.
                        format(_DEFAULT_TOOLCHAIN))

    parser.add_argument('--board-target',
                        default=_DEFAULT_BOARD_TARGET,
                        help='board type, default = {}'.
                        format(_DEFAULT_BOARD_TARGET))

    parser.add_argument('--build-targets',
                        default=_DEFAULT_BUILD_TARGETS,
                        nargs='+',
                        help='build targets, default = {}'.
                        format(_DEFAULT_BUILD_TARGETS))

    parser.add_argument('--build-type',
                        default=_DEFAULT_BUILD_TYPE,
                        choices=['Release', 'Debug', 'Coverage', 'Profile'],
                        help='build type, default = {}'.
                        format(_DEFAULT_BUILD_TYPE))

    parser.add_argument('--install-py2c', action='store_true',
                        help='install lib_py2c.so')

    parser.add_argument('--clean', action='store_true',
                        help='remove compiled artifacts')

    parser.add_argument('--scrub', action='store_true',
                        help='remove all build artifacts')

    parser.add_argument('--jobs', type=int,
                        default=_DEFAULT_BUILD_JOBS,
                        help='processes to use when building = {}'.
                        format(_DEFAULT_BUILD_JOBS))

    parser.add_argument('--lttng', action='store_true',
                        help='build with LTTNG flag set.'
                             ' Note: does not affect build type.')

    parser.add_argument('--verbose', action='store_true',
                        help='verbose builds')

    parser.add_argument('--version', action='store_true',
                        help='print the CMake version')

    parser.add_argument('-D',action='append',nargs='+',
                        help='Additional configuration variables')

    args = parser.parse_args()

    cmd_line_definitions = {}
    if args.D:
        for defn in args.D:
            (name,value) = defn[0].split('=')
            if not name or not value:
                err_msg = 'Bad name "{}" or value "{}". Var must be in the form of "-Dname=value".'.format(name,value)
                raise ValueError(err_msg)
            cmd_line_definitions[name] = value

    compiler_definitions = {}
    if args.lttng:
        compiler_definitions['LTTNG'] = 1

    sdk_builder = Ex10SdkBuilder(
        c_sdk_root_path=args.c_sdk_root,
        build_root_path=args.build_path,
        toolchain_file=args.toolchain,
        board_target=args.board_target,
        build_targets=args.build_targets,
        build_type=args.build_type,
        cmd_line_definitions=cmd_line_definitions,
        compiler_definitions=compiler_definitions,
        build_jobs=args.jobs,
        verbose=args.verbose)

    if args.scrub:
        sdk_builder.scrub()
        return

    if args.version:
        print('CMake version: {}'.format(sdk_builder.cmake_version_string))
        return

    if args.clean:
        sdk_builder.build_targets = ['clean']

    sdk_builder.configure()
    sdk_builder.build()

    if args.install_py2c:
        sdk_builder.install_py2c()


if __name__ == '__main__':
    try:
        main()
    except RuntimeError as error:
        sys.exit(1)
