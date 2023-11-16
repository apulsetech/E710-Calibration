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
Autogen context for calibration data
"""

import os

import python_tools.ex10_api_yaml_parsers.calibration_parser as \
    calibration_parser

CALIBRATION_YAML_PATH = os.path.join('dev_kit', 'ex10_dev_kit', 'cal', 'cal_yaml')

def get_context(filepath=CALIBRATION_YAML_PATH):
    """ The Calibration autogen hook """
    filename = os.path.join(filepath, 'cal_info_page_v5.yml')
    address_map = calibration_parser.load_address_map(filename)
    parameters_list = address_map.get_parameters()
    calibration_parser.address_map_check_overlap(parameters_list)

    parameters_name_len_max = 0
    for parameter in parameters_list:
        parameters_name_len_max = max(len(parameter.name),
                                      parameters_name_len_max)

    return {'parameters_list': parameters_list,
            'parameters_name_len_max': parameters_name_len_max}
