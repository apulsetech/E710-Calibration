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
IT IS STRONGLY RECOMMENDED THAT YOU HAVE A CLEAN WORKING COPY BEFORE RUNNING
THIS SCRIPT. This means that `git status` does not report any changed files.

Filenames(file path) in the _FILES_TO_CALGEN will have autogeneration run in
place and the files will be replaced with the new content. If you wish to
CalGen additional calibration files, add the filename to _FILES_TO_CALGEN.
Make sure you have the CalGen context modules(jinja2 and python files) for
each calibration file you would like to CalGen.

After running CalGen, use 'git status' to discover which files were re-generated
"""
from __future__ import division, absolute_import
from __future__ import print_function, unicode_literals

import argparse
import io
import os
import pkgutil
import re
import sys
import jinja2

# All file directories to CalGen should be added to this list
_FILES_TO_CALGEN = [
    r'/ex10_c_dev_kit/board/e710_ref_design/calibration_v5.h',
    r'/ex10_c_dev_kit/board/e710_ref_design/calibration_v5.c',
    r'/ex10_c_dev_kit/examples/print_calibration_v5.c',
    r'/ex10_dev_kit/py2c_interface/py2c_python_cal.py',
    ]

def _get_calgen_proj_root():
    """
    Returns the path of the root of the git repository containing the current
    working directory

    """
    curr_file_path = os.path.dirname(os.path.abspath(__file__))
    proj_root = os.path.join(curr_file_path, os.pardir)
    proj_root = os.path.normpath(proj_root)
    return proj_root.rstrip(os.sep)

PROJ_ROOT = _get_calgen_proj_root()
MOD_DIR = os.path.join(PROJ_ROOT, 'ex10_calgen', 'content_template')
YAML_FILEPATH = os.path.join(PROJ_ROOT, 'ex10_cal', 'cal_yaml')

def set_calgen_proj_root(new_proj_root):
    """Set CalGen project root"""
    global PROJ_ROOT
    PROJ_ROOT = new_proj_root

    new_mod_dir = os.path.join(PROJ_ROOT, 'ex10_calgen', 'content_template')
    new_yaml_filepath = os.path.join(PROJ_ROOT, 'ex10_cal', 'cal_yaml')
    set_calgen_mod_dir(new_mod_dir)
    set_calgen_yaml_filepath(new_yaml_filepath)
    return

def set_calgen_mod_dir(new_mod_dir):
    """Set CalGen context module directory"""
    global MOD_DIR
    MOD_DIR = new_mod_dir
    return

def set_calgen_yaml_filepath(new_yaml_filepath):
    """Set calibration yaml filepath"""
    global YAML_FILEPATH
    YAML_FILEPATH = new_yaml_filepath
    return


class InvalidCalgenException(Exception):
    """
    An exception which is raised when an error is detected
    in an CalGen statement

    """
    pass

class CalGenBlock(object):

    def __init__(self, filename, descriptor, start_index, end_index):
        self.filename = filename
        self.start_index = start_index
        self.end_index = end_index

        block_arity = descriptor.count('|')

        if block_arity is not 1:
            raise InvalidCalgenException(
                'CalGen block arity mismatch.\n'
                'Blocks must be in the form:\n'
                'Impinj_calgen | <CalGen context> { '
            )

        desc_parts = descriptor.replace("{", "").split('|')

        self.calgen_module = desc_parts[1].strip()
        self.args = {}

    def __str__(self):
        return '\n'.join([
            20*'-',
            'CalGen Module: {}'.format(self.calgen_module),
            'File:{}'.format(self.filename),
            'Lines: {} - {}'.format(self.start_index + 1, self.end_index + 1),
            'args: {}'.format(self.args),
            '',
        ])

def _expand_calgen_blocks(filename, mod_directories):
    """
    Expands all CalGen blocks in a file

    :param filename: string, The file to be expanded
    :param mod_directories: A list of directories containing jinja2 templates
    and/or CalGen context modules.

    """
    _blocks, non_calgen_blocks = _extract_blocks_and_lines(filename)

    # Bail if there isn't anything to do
    # this is true if the closing CalGen indication is missing
    if len(_blocks) == 0:
        return None

    # creating a {name: importer} module map from mod_directories
    import_map = {package_name: importer for (importer, package_name, _) in
                  pkgutil.iter_modules(mod_directories)}

    expanded_file = io.StringIO()

    for idx, block in enumerate(_blocks):
        # Load the CalGen module (python file)
        ag_mod = import_map[block.calgen_module].find_module(
            block.calgen_module).load_module(block.calgen_module)

        # Get the jinja2 context from the module
        context = ag_mod.get_context(YAML_FILEPATH)

        env = jinja2.Environment(
            loader=jinja2.FileSystemLoader(mod_directories),
            trim_blocks=False)

        # Get the jinja2 template
        template = env.get_template('{}.jinja2'.format(block.calgen_module))

        # Render the block, then strip out all trailing whitespace
        block_content = '\n'.join(
            [line.rstrip() for line in template.render(**context).splitlines()])

        expanded_file.write(non_calgen_blocks[idx])
        expanded_file.write(block_content)

        # Handle the case where a template lacks a trailing newline
        if not block_content or block_content[-1] != '\n':
            expanded_file.write('\n')

    # It's possible that there are additional non CalGen blocks
    # after the CalGen portions, write each of them to the output file
    for na_block in non_calgen_blocks[len(_blocks):]:
        expanded_file.write(na_block)

    # Overwrite the original file with the newly expanded file
    with open(filename, 'w') as filehandle:
        filehandle.write(expanded_file.getvalue())

    return len(_blocks)


def _extract_blocks_and_lines(filename):
    """
    Extracts all autogenerator segments from a file

    :param filename: string, The path to the file containing CalGen segments
    :return: tuple, blocks: A list of CalGenBlock objects in the order they
     appear in the filenon _ag_bocks: A list of hand generated text blocks in
     the order they appear in the file

    """

    flines = open(filename).readlines()
    block_start_idx = None
    block_end_idx = None

    blocks = []
    non_calgen_blocks = []
    curr_non_calgen_block = ''

    # Walk through each line in the file and process it as either hand
    # generated or as part of an CalGen block
    for idx, line in enumerate(flines):
        stripped_line = line.strip()

        if 'Impinj_calgen' not in line:
            if block_start_idx is None:
                curr_non_calgen_block += line
            continue

        if stripped_line[-1] == '{':
            if block_start_idx is None:
                curr_non_calgen_block += line
                non_calgen_blocks.append(curr_non_calgen_block)
                curr_non_calgen_block = ''

                block_start_idx = idx
        elif stripped_line[-1] == '}':
            curr_non_calgen_block += line
            if block_start_idx is None:
                raise InvalidCalgenException(''.join([
                    'CalGen End statement with no beginning at ',
                    '{}:{}'.format(filename, idx)]))

            block_end_idx = idx
        else:
            raise InvalidCalgenException(''.join([
                'CalGen Error - {}:{}: missing end character'.format(
                    filename, idx)]))

        if (block_start_idx is not None) and (block_end_idx is not None):
            blocks.append(CalGenBlock(filename, flines[block_start_idx],
                                      block_start_idx, block_end_idx))
            block_start_idx = None
            block_end_idx = None

    non_calgen_blocks.append(curr_non_calgen_block)

    return blocks, non_calgen_blocks

def file_should_calgen(abs_path):
    """
    Given a filename, report whether it should be generated

    The given filename (abs_path) should be a normalized absolute path to the
    file in question.

    If the file happens to be a symlink, it's automatically ignored. Then,
    it's checked to see if there is a matching filename in the _FILES_TO_CALGEN
    list. If there is a match, the file is read to see if it contains at least
    one CalGen block, meaning a comment with 'Impinj_calgen' followed by a mod
    filename is found.

    :param abs_path: string, Path to the file
    :return: boolean, True is the file should use CalGen, false otherwise

    """
    if not os.path.exists(abs_path):
        print("No such file or directory: ", abs_path,
              "\nCheck if the project root directory is set correctly. "
              "See 'python run_inline_calgen.py --help' for more help. ")
        return False

    if os.path.islink(abs_path):
        return False

    # Checking whether the given filename is in the _FILES_TO_CALGEN list
    for filepath in _FILES_TO_CALGEN:
        norm_filepath = os.path.normpath(filepath)
        if norm_filepath in abs_path:
            with open(abs_path) as ifile:
                # Checking whether the given file has a CalGen block
                if re.search(r'Impinj_calgen \| .* \{', ifile.read()):
                    return True
                else:
                    print('Ignoring {}'.format(abs_path))

    return False

def inline_calgen_a_file(filename):
    """
    CalGen a single file
    :param abs_path: path to the file
    :param proj_root: project root directory
    :param mod_dir: directory of CalGen modules
    :return:
    """
    if file_should_calgen(filename):
        # Collecting all directories of jinja2 and py pairs
        alldirs = []
        listOfFiles = os.listdir(MOD_DIR)
        alldirs.append(MOD_DIR)
        for entry in listOfFiles:
            fullpath = os.path.join(MOD_DIR, entry)
            if os.path.isdir(fullpath):
                alldirs.append(fullpath)

        # Calgen a single file
        print('Autogenerating {}'.format(filename))
        _expand_calgen_blocks(filename, alldirs)

    return

def inline_calgen_all_the_things():
    """
    Walk through all the files in the _FILES_TO_CALGEN list and
    CalGen each file.
    """
    for the_file in _FILES_TO_CALGEN:
        # Getting the absolute path of the current file
        the_path_list = the_file.split('/')
        the_path = os.path.join(*the_path_list)
        abs_path = os.path.abspath(os.path.join(PROJ_ROOT, the_path))

        inline_calgen_a_file(abs_path)


def main():
    """
    Good ol' main
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', '-r',
                        help="specify the directory of the project root")
    args = parser.parse_args()

    if args.root:
        set_calgen_proj_root(args.root)

    inline_calgen_all_the_things()
    print("---------------------CALGEN COMPLETED---------------------")
    return

if __name__ == '__main__':
    sys.exit(main())
