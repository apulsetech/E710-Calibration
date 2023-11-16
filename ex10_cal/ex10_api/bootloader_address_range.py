#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2019 - 2021 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
"""
The bootloader register map addressing schema.
"""

# IPJ_autogen | gen_boot_ex10_api_py {
BOOTLOADER_ADDRESS_RANGE = {
    'CommandResult' : {
        'address' : 0x0000,
        'length'  : 0x0004,
        'access'  : 'read-only',
        'fields'  : {
            'FailedResultCode' : {
                'pos'   :  0,
                'bits'  :  8,
                'enums' : {
                    'Success'                    : 165,
                    'CommandInvalid'             : 1,
                    'ArgumentInvalid'            : 2,
                    'ResponseOverflow'           : 6,
                    'CommandMalformed'           : 7,
                    'AddressWriteFailure'        : 8,
                    'ImageInvalid'               : 9,
                    'LengthInvalid'              : 10,
                    'UploadStateInvalid'         : 11,
                    'BadCrc'                     : 14,
                    'FlashInvalidPage'           : 15,
                    'FlashPageLocked'            : 16,
                    'FlashEraseFailure'          : 17,
                    'FlashProgramFailure'        : 18,
                    'StoredSettingsMalformed'    : 19,
                    'NotEnoughSpace'             : 20,
                },
            },
            'FailedCommandCode' : {
                'pos'   :  8,
                'bits'  :  8,
                'enums' : {
                    'Read'                       : 1,
                    'Write'                      : 2,
                    'ReadFifo'                   : 3,
                    'StartUpload'                : 4,
                    'ContinueUpload'             : 5,
                    'CompleteUpload'             : 6,
                    'ReValidateMainImage'        : 7,
                    'Reset'                      : 8,
                    'TestTransfer'               : 10,
                    'WriteInfoPage'              : 11,
                    'TestRead'                   : 12,
                    'InsertFifoEvent'            : 14,
                },
            },
            'CommandsSinceFirstError' : {
                'pos'   : 16,
                'bits'  : 16,
            },
        },
    },
    'ResetCause' : {
        'address' : 0x0004,
        'length'  : 0x0002,
        'access'  : 'read-only',
        'fields'  : {
            'SoftwareReset' : {
                'pos'   :  0,
                'bits'  :  1,
            },
            'WatchdogTimeout' : {
                'pos'   :  1,
                'bits'  :  1,
            },
            'Lockup' : {
                'pos'   :  2,
                'bits'  :  1,
            },
            'ExternalReset' : {
                'pos'   :  3,
                'bits'  :  1,
            },
        },
    },
    'Status' : {
        'address' : 0x0006,
        'length'  : 0x0002,
        'access'  : 'read-only',
        'fields'  : {
            'Status' : {
                'pos'   :  0,
                'bits'  :  2,
                'enums' : {
                    'Bootloader'                 : 1,
                    'Application'                : 2,
                },
            },
        },
    },
    'VersionString' : {
        'address' : 0x0008,
        'length'  : 0x0020,
        'access'  : 'read-only',
    },
    'BuildNumber' : {
        'address' : 0x0028,
        'length'  : 0x0004,
        'access'  : 'read-only',
    },
    'GitHash' : {
        'address' : 0x002C,
        'length'  : 0x0004,
        'access'  : 'read-only',
    },
    'RamImageReturnValue' : {
        'address' : 0x0030,
        'length'  : 0x0004,
        'access'  : 'read-only',
        'fields'  : {
            'data' : {
                'pos'   :  0,
                'bits'  : 32,
            },
        },
    },
    'FrefFreq' : {
        'address' : 0x0034,
        'length'  : 0x0004,
        'access'  : 'read-write',
        'fields'  : {
            'FrefFreqKhz' : {
                'pos'   :  0,
                'bits'  : 32,
            },
        },
    },
    'RemainReason' : {
        'address' : 0x0038,
        'length'  : 0x0001,
        'access'  : 'read-only',
        'fields'  : {
            'RemainReason' : {
                'pos'   :  0,
                'bits'  :  8,
                'enums' : {
                    'NoReason'                   : 0,
                    'ReadyNAsserted'             : 1,
                    'ApplicationImageInvalid'    : 2,
                    'ResetCommand'               : 3,
                    'Crash'                      : 4,
                    'Watchdog'                   : 5,
                    'Lockup'                     : 6,
                },
            },
        },
    },
    'ImageValidity' : {
        'address' : 0x0039,
        'length'  : 0x0001,
        'access'  : 'read-only',
        'fields'  : {
            'ImageValidMarker' : {
                'pos'   :  0,
                'bits'  :  1,
            },
            'ImageNonValidMarker' : {
                'pos'   :  1,
                'bits'  :  1,
            },
        },
    },
    'BootloaderVersionString' : {
        'address' : 0x003A,
        'length'  : 0x0020,
        'access'  : 'read-only',
    },
    'BootloaderBuildNumber' : {
        'address' : 0x005A,
        'length'  : 0x0004,
        'access'  : 'read-only',
    },
    'BootloaderGitHash' : {
        'address' : 0x005E,
        'length'  : 0x0004,
        'access'  : 'read-only',
    },
    'SerialNumber' : {
        'address' : 0x0070,
        'length'  : 0x0020,
        'access'  : 'read-only',
    },
    'CrashInfo' : {
        'address' : 0x0100,
        'length'  : 0x0100,
        'access'  : 'read-only',
    },
}
# IPJ_autogen }
