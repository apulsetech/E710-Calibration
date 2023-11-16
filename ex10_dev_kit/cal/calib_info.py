#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2019 - 2023 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
This is a class to read and write the OEM calibration info page on Impinj reader chip
"""

from __future__ import division, absolute_import
from __future__ import print_function, unicode_literals

import json
import struct
import os
import pprint
from enum import IntEnum
import six
import yaml


class CalInfoEnum(IntEnum):
    # pylint: disable=locally-disabled, bad-whitespace
    """ Provides state of the Calibration info """
    NotRead      = 0
    InfoGood     = 1
    InfoCorrupt  = 2
    InfoNotFound = 3
    NotSupported = 4


class CalibrationInfoPageAccessor(object):
    """
    This is a class to read and write the OEM calibration info page on Impinj reader chip
    """

    # these are the filename for the yaml files that define the
    # format of the data in the info page.  The first byte of the
    # first 32 bits of the info page designates what format it is
    _INFO_PAGE_YAML_FILES = (
        b'dummy_page.yml',
        b'dummy_page.yml',
        b'dummy_page.yml',
        b'dummy_page.yml',
        b'dummy_page.yml',
        b'cal_yaml/cal_info_page_v5.yml',
    )

    def __init__(self):
        """
        This object initializes the cal parameters to being empty
        It needs to be loaded by one of the methods below
        """
        self.cal_param = []
        self.cal_version = 0
        self.cal_supported_versions = [3, 4, 5]
        self.cal_valid = CalInfoEnum.NotRead.value

    @staticmethod
    def build_struct_string(fields):
        """
        Helper function to build up a struct format string based on the
        fields parameters.
        """
        struct_string = b'<'
        num_bytes = 0

        for field in fields:
            num_entries = field.get('num_entries', 1)
            if field['bits'] == 8:
                struct_string += b'B' * num_entries
                num_bytes += 1 * num_entries
            elif field['bits'] == 16:
                struct_string += b'H' * num_entries
                num_bytes += 2 * num_entries
            elif field['bits'] == 32:
                struct_string += b'I' * num_entries
                num_bytes += 4 * num_entries
            elif field['bits'] == 'short':
                struct_string += b'h' * num_entries
                num_bytes += 2 * num_entries
            elif field['bits'] == 'int':
                struct_string += b'i' * num_entries
                num_bytes += 4 * num_entries
            elif field['bits'] == 'float':
                struct_string += b'f' * num_entries
                num_bytes += 4 * num_entries
            elif field['bits'] == 'double':
                struct_string += b'd' * num_entries
                num_bytes += 8 * num_entries
            else:
                raise RuntimeError('Unsupported bit field width')

        return struct_string, num_bytes

    def read_in_yaml(self, yaml_version):
        """
        Helper function to read in the yaml file and setup the cal_parameters
        based on the data in the calibration file
        """
        # ugly little hack to figure out where this module is installed
        # so that we can open the file relative to that.  If there is a
        # better way that will work in yk_design, and a package please
        # speak up!
        filename = os.path.join(
            os.path.dirname(__file__),
            self._INFO_PAGE_YAML_FILES[yaml_version].decode())

        with open(filename, 'r') as yaml_file:
            cal_yaml = yaml.safe_load(yaml_file)

        # clear out the cal params in case we are reloading to a different
        # version
        self.cal_param = []
        for parameter in cal_yaml['parameters']:
            values = []
            for field in parameter['fields']:
                num_entries = field.get('num_entries', 1)
                values.extend([field['init_value']] * num_entries)
            parameter['value'] = tuple(values)
            self.cal_param.extend([parameter, ])

    def from_info_page_string(self, bytestream):
        """
        A helper function to read the data into the structure from the info
        page stream
        """

        if bytestream is None or bytestream[0] == 255:
            self.cal_valid = CalInfoEnum.InfoNotFound.value
            return

        # read the first byte to determine which yaml file to load
        if six.PY2:
            version = struct.unpack('B', bytestream[0])
        if six.PY3:
            version = int(bytestream[0])
        self.cal_version = version

        if version not in self.cal_supported_versions:
            self.cal_valid = CalInfoEnum.NotSupported.value
            return

        print('Using calibration version {}'.format(self.cal_version))

        self.read_in_yaml(version)

        for idx, parameter in enumerate(self.cal_param):
            location = parameter['address']
            if location > len(bytestream):
                self.cal_valid = CalInfoEnum.InfoCorrupt.value
                return

            struct_str, _ = self.build_struct_string(parameter['fields'])

            values = struct.unpack_from(struct_str, bytestream[location:])
            self.cal_param[idx]['value'] = values

        self.cal_valid = CalInfoEnum.InfoGood.value

    def to_info_page_string(self):
        """
        write out the contents of self.cal_param into a byte stream that
        can be stored in the info page. Failure to pack the bytestream results
        in an exception. This halts the process (does not write a corrupt
        info page)
        """

        bytestream = b''

        if self.cal_param == {}:
            raise RuntimeError('No calibration parameters to dump')

        for parameter in self.cal_param:
            address = parameter['address']
            if address > len(bytestream):
                bytestream += b'\x00' * (address - len(bytestream))
            struct_str, _ = self.build_struct_string(parameter['fields'])
            try:
                bytestream += struct.pack(struct_str, *parameter['value'])
            except struct.error:
                raise Exception('Error in to_info_page_string')
        if len(bytestream) > 2048:
            raise RuntimeError('info page output too large')

        return bytestream

    def to_json(self, filename):
        """
        Save the cal parameters to a json file
        """
        dirname = os.path.dirname(filename)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        with open(filename, 'w+') as outfile:
            outfile.seek(0)
            json.dump(self.cal_param, outfile, indent=4, sort_keys=True)

    def from_json(self, filename):
        """
        Read in the cal parameters from a json file
        """
        with open(filename, 'r') as infile:
            self.cal_param = json.load(infile)

    def get_parameter(self, name):
        """
        This helper function will retrieve the value for the named
        parameter
        """
        for param in self.cal_param:
            if param['name'] == name:
                return param['value']

        raise KeyError('name {} not in cal parameters: {}'.
                       format(name, pprint.pformat(self.cal_param)))

    def set_parameter(self, name, value):
        """
        This helper function will set the value for the named parameter
        It also checks to see that the value is a tuple and has the
        proper number of elements (according to the yaml file description)
        """
        for idx, param in enumerate(self.cal_param):
            if param['name'] == name:
                # Each field may have a 'num_entries' dictionary entry.
                # If it does not then the default is one. The length of the
                # value iterable to write must match the total number of
                # entries for the calibration parameter.
                field_entries = 0
                for field in param['fields']:
                    field_entries += field.get('num_entries', 1)
                if len(value) != field_entries:
                    raise ValueError(
                        'Incorrect number of elements {} for parameter: {}: '
                        'field_entries: {}'.format(
                            len(value), param['name'], field_entries))

                self.cal_param[idx]['value'] = value

    def dump_params(self):
        """
        This function dumps all cal parameter values into a list of tuples
        containing strings of the param name and value
        """
        param_dump = [(0,)] * len(self.cal_param)
        for idx, param in enumerate(self.cal_param):
            param_dump[idx] = (param['name'], param['value'])
        return param_dump
