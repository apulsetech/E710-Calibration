#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2020 - 2023 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
Parse the calibration yaml file.
"""

from __future__ import division, absolute_import
from __future__ import print_function, unicode_literals

import os
import re
import pprint
import argparse
import six
import yaml

from python_tools.parsing.parsing_utils import _indent, camel_to_snake


CALIBRATION_YAML_PATH = os.path.join(
    'dev_kit', 'ex10_dev_kit', 'cal', 'cal_yaml')
CALIBRATION_YAML_FILE = os.path.join(
    CALIBRATION_YAML_PATH, 'cal_info_page_v5.yml')


class Node(object):
    # pylint: disable=locally-disabled, missing-function-docstring
    # pylint: disable=locally-disabled, too-few-public-methods
    """
    The root node object.
    All objects inherit from Node so that they can contain each other.
    """
    def __init__(self, parent, self_dict):
        self._parent = parent
        self._self_dict = self_dict
        self._name = None
        self._brief = self._get_optional_field(six.text_type, 'brief', '')
        self._description = self._get_optional_field(
            six.text_type, 'description', '')

    @property
    def dbg_name(self):
        # pylint: disable=locally-disabled, missing-function-docstring
        if self._name:
            return '.'.join((self._parent.dbg_name, self._name))

        return ' in '.join(((six.text_type(type(self)) +
                             pprint.pformat(self._self_dict)),
                            self._parent.dbg_name))

    def _get_required_field(self, klass, key):
        # pylint: disable=locally-disabled, missing-function-docstring
        try:
            return klass(self._self_dict[key])
        except (ValueError, TypeError):
            raise ValueError('{} in {} is not a {}'
                             .format(key, self.dbg_name, six.text_type(klass)))
        except KeyError:
            raise ValueError('{} is missing a "{}" entry'
                             .format(self.dbg_name, key))

    def _get_optional_field(self, klass, key, default):
        # pylint: disable=locally-disabled, missing-function-docstring
        try:
            return klass(self._self_dict.get(key, default))
        except (ValueError, TypeError):
            raise ValueError('{} in {} is not a {}'
                             .format(key, self.dbg_name, six.text_type(klass)))

    def _parse_optional_node_list(self, klass, key):
        """
        From a dictionary entry parse the dictionary entries into a list of
        classes. The original motivation for this class is to consolidate the
        parsing of 'enums' and 'fields' entries.

        Example: The YAML entry:
            fields:
              - name: Status
                pos: 0
                bits: 1
        will return a list containing one element of the Fields class given
        the call: _get_optional_field(Fields, 'fields')
        """
        entry_list = self._get_optional_field(list, key, [])
        entries = [klass(self, entry_dict) for entry_dict in entry_list]
        return entries


class Field(Node):
    """
    Allows for the register to be segmented into bit-fields.
    Note that fields may contain enum types but cannot be at the same level
    of heirarchy as an enum type.
    """
    def __init__(self, parent, field_dict):
        super(Field, self).__init__(parent, field_dict)

        self._name = self._get_required_field(six.text_type, 'name')
        self._name_snake = camel_to_snake(self._name)
        self._pos = self._get_required_field(int, 'pos')

        # If a field has num_entries > 0 then it is allocated an array of size
        # num_entries.
        self._num_entries = self._get_optional_field(int, 'num_entries', 0)

        # set uint as default, explicitly state other types in the input file
        self._resolve_as = self._get_optional_field(str, 'resolve_as', '')

        try:
            self._bits = self._get_required_field(int, 'bits')
            if not self._resolve_as:
                self._resolve_as = 'uint{}_t'.format(self._bits)
        except ValueError as error:
            bits = self._get_required_field(str, 'bits')
            if bits == 'float':
                self._bits = 32
                self._resolve_as = 'float'
            elif bits == 'double':
                self._bits = 64
                self._resolve_as = 'double'
            elif bits == 'short':
                self._bits = 16
                self._resolve_as = 'int16_t'
            elif bits == 'int':
                self._bits = 32
                self._resolve_as = 'int32_t'
            else:
                raise TypeError(error)

        if self.resolve_as == 'float' or self.resolve_as == 'double':
            self._init_value = self._get_optional_field(float, 'init_value', 0)
        else:
            self._init_value = self._get_optional_field(int, 'init_value', 0)

    def __str__(self):
        string = ''
        string += 'name: {}'.format(self.name) + os.linesep
        if self.brief:
            string += 'brief: {}'.format(self.brief) + os.linesep
        string += 'pos:  {:2d}'.format(self.pos) + os.linesep
        string += 'bits: {:2d}'.format(self.bits) + os.linesep
        if self.resolve_as:
            string += 'resolve_as: {}'.format(self.resolve_as) + os.linesep
        if self.description:
            string += 'description:' + os.linesep + _indent(self.description)
        string = string.strip() + os.linesep
        return string

    # pylint: disable=locally-disabled, missing-function-docstring
    @property
    def name(self):
        return self._name

    @property
    def name_snake(self):
        return self._name_snake

    @property
    def brief(self):
        return self._brief

    @property
    def description(self):
        return self._description

    @property
    def pos(self):
        return self._pos

    @property
    def bits(self):
        return self._bits

    @property
    def init_value(self):
        return self._init_value

    @property
    def resolve_as(self):
        return self._resolve_as

    @property
    def num_entries(self):
        return self._num_entries


class Entry(Node):
    """
    The top-most level of Yukon register entry.
    """
    def __init__(self, parent, entry_dict):
        super(Entry, self).__init__(parent, entry_dict)

        self._name = self._get_required_field(six.text_type, 'name')
        self._name_snake = camel_to_snake(self._name)

        name_agnostic, use_this_decl = self._convert_band_agnostic(self._name)
        self._name_band_agnostic = name_agnostic
        self._use_this_decl = use_this_decl
        self._address = self._get_required_field(int, 'address')
        self._length = self._get_required_field(int, 'length')
        self._fields = self._parse_optional_node_list(Field, 'fields')

    def __str__(self):
        string = ''
        string += 'name:      {}'.format(self.name) + os.linesep
        if self.brief:
            string += 'brief:     {}'.format(self.brief) + os.linesep
        string += 'address:   0x{:04x}'.format(self.address) + os.linesep
        string += 'length:    0x{:04x}'.format(self.length) + os.linesep
        if self.fields:
            string += 'fields:' + os.linesep
            for field in self.fields:
                string += _indent('{}'.format(field))
            string = string.strip() + '' + os.linesep
        if self.description:
            string += 'description:' + os.linesep
            string += '{}'.format(_indent(self.description))
            string = string.strip() + os.linesep
        return string

    def range_string(self):
        """ Return a string representing an entry name and address range """
        return '{}: [0x{:04X}:0x{:04X}]'.format(
            self.name, self.address, self.address + self.length - 1)

    def _parse_optional_dict(self, klass, key):
        """
        Parse an optional dictionary.
        """
        dictionary = self._get_optional_field(dict, key, {})
        return klass(self, dictionary)

    @staticmethod
    def _convert_band_agnostic(name):
        """
        Remove 'UpperBand', 'LowerBand' from name and replace it with the
        band agnostic string 'PerBand'
        """
        use_this_decl = True
        agnostic = 'PerBand'
        match = re.search(pattern='UpperBand', string=name)
        if match is None:
            match = re.search(pattern='LowerBand', string=name)
            use_this_decl = False
        if match is not None:
            (beg, end) = match.span()
            name_1 = name[:beg]
            name_2 = name[end:]
            return agnostic + name_1 + name_2, use_this_decl

        use_this_decl = True
        return name, use_this_decl

    # pylint: disable=locally-disabled, missing-function-docstring
    @property
    def name(self):
        return self._name

    @property
    def name_snake(self):
        return self._name_snake

    @property
    def name_band_agnostic(self):
        return self._name_band_agnostic

    @property
    def use_this_decl(self):
        return self._use_this_decl

    @property
    def brief(self):
        return self._brief

    @property
    def description(self):
        return self._description

    @property
    def address(self):
        return self._address

    @property
    def length(self):
        return self._length

    @property
    def fields(self):
        return self._fields


def address_map_check_overlap(address_map):
    """
    Check the address range for overlapping entries.
    Raise a ValueError if overlaps are found.
    """
    entry_prev = None
    for entry in address_map:
        if entry_prev is not None:
            if entry.address < entry_prev.address + entry_prev.length:
                raise ValueError('overlap found: {}, {}'.format(
                    entry_prev.range_string(), entry.range_string()))

        entry_prev = entry


class AddressMap(Node):
    """ The Calibration 'parameters' fields as parsed in to Entry nodes """
    def __init__(self, yaml_dict):
        super(AddressMap, self).__init__(None, yaml_dict)

        self._brief = self._get_required_field(six.text_type, 'brief')
        self._parameters = self._parse_entries(
            self._get_required_field(list, 'parameters'))

    def _parse_entries(self, entries_list):
        entries = [Entry(self, entry_dict) for entry_dict in entries_list]
        return entries

    @property
    def brief(self):
        # pylint: disable=locally-disabled, missing-function-docstring
        return self._brief

    def get_parameters(self):
        """
        Get all calibration entries that are within the parameter list
        sorted by address in ascending order.
        """
        return sorted(self._parameters, key=lambda x: x.address)

    @property
    def dbg_name(self):
        # pylint: disable=locally-disabled, missing-function-docstring
        return 'MapTop'


def load_address_map(yaml_filename):
    # pylint: disable=locally-disabled, missing-function-docstring
    with open(yaml_filename) as input_file:
        return AddressMap(yaml.safe_load(input_file))


def main():
    """
    main() used for debugging and examining the Calibration yaml parser.
    """
    parser = argparse.ArgumentParser(description='Ex10 Calibration Parser')

    parser.add_argument('-f', '--file', type=six.text_type,
                        default=CALIBRATION_YAML_FILE,
                        help='specify the Calibration yaml file, '
                        'default: {}'.format(CALIBRATION_YAML_FILE))

    args = parser.parse_args()

    calibration_map = load_address_map(args.file)
    parameters_map = calibration_map.get_parameters()
    address_map_check_overlap(parameters_map)

    for node in parameters_map:
        print(node)


if __name__ == '__main__':
    main()
