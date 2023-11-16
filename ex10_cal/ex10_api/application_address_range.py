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
The application register map addressing schema.
"""
# pylint: disable=locally-disabled, too-many-lines

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)

# IPJ_autogen | gen_py_app_ex10_api {
APPLICATION_ADDRESS_RANGE = {
    'CommandResult' : {
        'address'    : 0x0000,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'FailedResultCode' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'enum',
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
                'pos'       :  8,
                'bits'      :  8,
                'resolve_as': 'enum',
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
                'pos'       : 16,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'ResetCause' : {
        'address'    : 0x0004,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'SoftwareReset' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'WatchdogTimeout' : {
                'pos'       :  1,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Lockup' : {
                'pos'       :  2,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'ExternalReset' : {
                'pos'       :  3,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'Status' : {
        'address'    : 0x0006,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'Status' : {
                'pos'       :  0,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Bootloader'                 : 1,
                    'Application'                : 2,
                },
            },
        },
    },
    'VersionString' : {
        'address'    : 0x0008,
        'length'     : 0x0020,
        'num_entries': 1,
        'access'     : 'read-only',
    },
    'BuildNumber' : {
        'address'    : 0x0028,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
    },
    'GitHash' : {
        'address'    : 0x002C,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
    },
    'Timestamp' : {
        'address'    : 0x0030,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'CurrentTimestampUs' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'FrefFreq' : {
        'address'    : 0x0034,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'FrefFreqKhz' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint32_t',
            },
        },
    },
    'ProductSku' : {
        'address'    : 0x0068,
        'length'     : 0x0008,
        'num_entries': 1,
        'access'     : 'read-only',
    },
    'SerialNumber' : {
        'address'    : 0x0070,
        'length'     : 0x0020,
        'num_entries': 1,
        'access'     : 'read-only',
    },
    'DeviceInfo' : {
        'address'    : 0x0090,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'EcoRevision' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'DeviceRevisionLo' : {
                'pos'       :  8,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'DeviceRevisionHi' : {
                'pos'       : 16,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'DeviceIdentifier' : {
                'pos'       : 24,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
        },
    },
    'DeviceBuild' : {
        'address'    : 0x0094,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'SparRevision' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'RtlBuildNumberLo' : {
                'pos'       :  8,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'RtlBuildNumberHi' : {
                'pos'       : 16,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
        },
    },
    'RtlRevision' : {
        'address'    : 0x0098,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'RtlRevision' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'StackDepth' : {
        'address'    : 0x009C,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'depth' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'InterruptMask' : {
        'address'    : 0x00A0,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'OpDone' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Halted' : {
                'pos'       :  1,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'EventFifoAboveThresh' : {
                'pos'       :  2,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'EventFifoFull' : {
                'pos'       :  3,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'InventoryRoundDone' : {
                'pos'       :  4,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'HaltedSequenceDone' : {
                'pos'       :  5,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'CommandError' : {
                'pos'       :  6,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AggregateOpDone' : {
                'pos'       :  7,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'InterruptMaskSet' : {
        'address'    : 0x00A4,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'write-only',
        'fields'  : {
            'OpDone' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Halted' : {
                'pos'       :  1,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'EventFifoAboveThresh' : {
                'pos'       :  2,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'EventFifoFull' : {
                'pos'       :  3,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'InventoryRoundDone' : {
                'pos'       :  4,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'HaltedSequenceDone' : {
                'pos'       :  5,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'CommandError' : {
                'pos'       :  6,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AggregateOpDone' : {
                'pos'       :  7,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'InterruptMaskClear' : {
        'address'    : 0x00A8,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'write-only',
        'fields'  : {
            'OpDone' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Halted' : {
                'pos'       :  1,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'EventFifoAboveThresh' : {
                'pos'       :  2,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'EventFifoFull' : {
                'pos'       :  3,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'InventoryRoundDone' : {
                'pos'       :  4,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'HaltedSequenceDone' : {
                'pos'       :  5,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'CommandError' : {
                'pos'       :  6,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AggregateOpDone' : {
                'pos'       :  7,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'InterruptStatus' : {
        'address'    : 0x00AC,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'OpDone' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Halted' : {
                'pos'       :  1,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'EventFifoAboveThresh' : {
                'pos'       :  2,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'EventFifoFull' : {
                'pos'       :  3,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'InventoryRoundDone' : {
                'pos'       :  4,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'HaltedSequenceDone' : {
                'pos'       :  5,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'CommandError' : {
                'pos'       :  6,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AggregateOpDone' : {
                'pos'       :  7,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'EventFifoNumBytes' : {
        'address'    : 0x00B0,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'NumBytes' : {
                'pos'       :  0,
                'bits'      : 12,
                'resolve_as': 'uint',
            },
        },
    },
    'EventFifoIntLevel' : {
        'address'    : 0x00B2,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Threshold' : {
                'pos'       :  0,
                'bits'      : 12,
                'resolve_as': 'uint',
            },
        },
    },
    'GpioOutputEnable' : {
        'address'    : 0x00B4,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'EnableBits' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'GpioOutputLevel' : {
        'address'    : 0x00B8,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'LevelBits' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'PowerControlLoopAuxAdcControl' : {
        'address'    : 0x00BC,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'ChannelEnableBits' : {
                'pos'       :  0,
                'bits'      : 15,
                'resolve_as': 'enum',
                'enums' : {
                    'None'                       : 0,
                    'PowerLo0'                   : 1,
                    'PowerLo1'                   : 2,
                    'PowerLo2'                   : 4,
                    'PowerLo3'                   : 8,
                    'PowerRx0'                   : 16,
                    'PowerRx1'                   : 32,
                    'PowerRx2'                   : 64,
                    'PowerRx3'                   : 128,
                    'TestMux0'                   : 256,
                    'TestMux1'                   : 512,
                    'TestMux2'                   : 1024,
                    'TestMux3'                   : 2048,
                    'Temperature'                : 4096,
                    'PowerLoSum'                 : 8192,
                    'PowerRxSum'                 : 16384,
                },
            },
        },
    },
    'PowerControlLoopGainDivisor' : {
        'address'    : 0x00C0,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'GainDivisor' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'PowerControlLoopMaxIterations' : {
        'address'    : 0x00C4,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'MaxIterations' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'PowerControlLoopAdcTarget' : {
        'address'    : 0x00CC,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'AdcTargetValue' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'PowerControlLoopAdcThresholds' : {
        'address'    : 0x00D0,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'LoopStopThreshold' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
            'OpErrorThreshold' : {
                'pos'       : 16,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'DelayUs' : {
        'address'    : 0x00D4,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Delay' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'GpioOutputLevelSet' : {
        'address'    : 0x00E0,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'write-only',
        'fields'  : {
            'LevelBitsSet' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'GpioOutputLevelClear' : {
        'address'    : 0x00E4,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'write-only',
        'fields'  : {
            'LevelBitsClear' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'GpioOutputEnableSet' : {
        'address'    : 0x00E8,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'write-only',
        'fields'  : {
            'EnableBitsSet' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'GpioOutputEnableClear' : {
        'address'    : 0x00EC,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'write-only',
        'fields'  : {
            'EnableBitsClear' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'OpsControl' : {
        'address'    : 0x0300,
        'length'     : 0x0001,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'OpId' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'enum',
                'enums' : {
                    'Idle'                       : 160,
                    'LogTestOp'                  : 161,
                    'MeasureAdcOp'               : 162,
                    'TxRampUpOp'                 : 163,
                    'TxRampDownOp'               : 164,
                    'SetTxCoarseGainOp'          : 165,
                    'SetTxFineGainOp'            : 166,
                    'RadioPowerControlOp'        : 167,
                    'SetRfModeOp'                : 168,
                    'SetRxGainOp'                : 169,
                    'LockSynthesizerOp'          : 170,
                    'EventFifoTestOp'            : 171,
                    'RxRunSjcOp'                 : 172,
                    'SetGpioOp'                  : 173,
                    'SetClearGpioPinsOp'         : 174,
                    'StartInventoryRoundOp'      : 176,
                    'RunPrbsDataOp'              : 177,
                    'SendSelectOp'               : 178,
                    'SetDacOp'                   : 179,
                    'SetATestMuxOp'              : 180,
                    'PowerControlLoopOp'         : 181,
                    'MeasureRssiOp'              : 182,
                    'UsTimerStartOp'             : 183,
                    'UsTimerWaitOp'              : 184,
                    'AggregateOp'                : 185,
                    'ListenBeforeTalkOp'         : 186,
                    'BerTestOp'                  : 192,
                    'EtsiBurstOp'                : 193,
                    'HpfOverrideTestOp'          : 194,
                    'SetDcOffsetOp'              : 196,
                },
            },
        },
    },
    'OpsStatus' : {
        'address'    : 0x0304,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'OpId' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'enum',
                'enums' : {
                    'Idle'                       : 160,
                    'LogTestOp'                  : 161,
                    'MeasureAdcOp'               : 162,
                    'TxRampUpOp'                 : 163,
                    'TxRampDownOp'               : 164,
                    'SetTxCoarseGainOp'          : 165,
                    'SetTxFineGainOp'            : 166,
                    'RadioPowerControlOp'        : 167,
                    'SetRfModeOp'                : 168,
                    'SetRxGainOp'                : 169,
                    'LockSynthesizerOp'          : 170,
                    'EventFifoTestOp'            : 171,
                    'RxRunSjcOp'                 : 172,
                    'SetGpioOp'                  : 173,
                    'SetClearGpioPinsOp'         : 174,
                    'StartInventoryRoundOp'      : 176,
                    'RunPrbsDataOp'              : 177,
                    'SendSelectOp'               : 178,
                    'SetDacOp'                   : 179,
                    'SetATestMuxOp'              : 180,
                    'PowerControlLoopOp'         : 181,
                    'MeasureRssiOp'              : 182,
                    'UsTimerStartOp'             : 183,
                    'UsTimerWaitOp'              : 184,
                    'AggregateOp'                : 185,
                    'ListenBeforeTalkOp'         : 186,
                    'BerTestOp'                  : 192,
                    'EtsiBurstOp'                : 193,
                    'HpfOverrideTestOp'          : 194,
                    'SetDcOffsetOp'              : 196,
                },
            },
            'Busy' : {
                'pos'       :  8,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Error' : {
                'pos'       : 16,
                'bits'      :  8,
                'resolve_as': 'enum',
                'enums' : {
                    'None'                       : 0,
                    'UnknownOp'                  : 1,
                    'UnknownError'               : 2,
                    'InvalidParameter'           : 3,
                    'PllNotLocked'               : 4,
                    'PowerControlTargetFailed'   : 5,
                    'InvalidTxState'             : 6,
                    'RadioPowerNotEnabled'       : 7,
                    'AggregateBufferOverflow'    : 8,
                    'AggregateInnerOpError'      : 9,
                    'SjcCdacRangeError'          : 11,
                    'SjcResidueThresholdExceeded' : 12,
                    'DroopCompensationTooManyAdcChannels' : 13,
                    'EventFailedToSend'          : 14,
                    'AggregateEx10CommandError'  : 15,
                    'UnsupportedCommand'         : 16,
                    'BerRxHung'                  : 17,
                },
            },
        },
    },
    'HaltedControl' : {
        'address'    : 0x0308,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Go' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Resume' : {
                'pos'       :  1,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'NakTag' : {
                'pos'       :  2,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'HaltedStatus' : {
        'address'    : 0x030C,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'Halted' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Busy' : {
                'pos'       :  1,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'rfu_1' : {
                'pos'       :  2,
                'bits'      :  6,
                'resolve_as': 'uint8_t',
            },
            'rfu_2' : {
                'pos'       :  8,
                'bits'      : 16,
                'resolve_as': 'uint16_t',
            },
            'Error' : {
                'pos'       : 24,
                'bits'      :  8,
                'resolve_as': 'enum',
                'enums' : {
                    'NoError'                    : 0,
                    'CoverCodeSizeError'         : 1,
                    'GetCoverCodeFailed'         : 2,
                    'BadCrc'                     : 3,
                    'Unknown'                    : 4,
                },
            },
        },
    },
    'LogTestPeriod' : {
        'address'    : 0x0320,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Period' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'LogTestWordRepeat' : {
        'address'    : 0x0324,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Repeat' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
            'TestType' : {
                'pos'       : 16,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'EventFifoTestPeriod' : {
        'address'    : 0x0328,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Period' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'EventFifoTestPayloadNumWords' : {
        'address'    : 0x032C,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'NumWords' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
        },
    },
    'LogSpeed' : {
        'address'    : 0x0330,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'SpeedMhz' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
        },
    },
    'LogEnables' : {
        'address'    : 0x0334,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'OpLogs' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'RampingLogs' : {
                'pos'       :  1,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'ConfigLogs' : {
                'pos'       :  2,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'LmacLogs' : {
                'pos'       :  3,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'SjcSolutionLogs' : {
                'pos'       :  4,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'RfSynthLogs' : {
                'pos'       :  5,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'PowerControlSolutionLogs' : {
                'pos'       :  6,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AuxLogs' : {
                'pos'       :  7,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'RegulatoryLogs' : {
                'pos'       :  8,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'CommandResponseLogs' : {
                'pos'       :  9,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'InsertFifoEventLogs' : {
                'pos'       : 10,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'HostIrqLogs' : {
                'pos'       : 11,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'TimerStartLogs' : {
                'pos'       : 12,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'TimerWaitLogs' : {
                'pos'       : 13,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AggregateOpLogs' : {
                'pos'       : 14,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'ReadFifoLogs' : {
                'pos'       : 15,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'LbtOpLogs' : {
                'pos'       : 16,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'rfu_1' : {
                'pos'       : 17,
                'bits'      :  7,
                'resolve_as': 'uint8_t',
            },
            'rfu_2' : {
                'pos'       : 24,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'WfeTrace' : {
                'pos'       : 25,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'RssiTraceLogs' : {
                'pos'       : 26,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'ModemDataLogs' : {
                'pos'       : 27,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'CodeCoverage' : {
                'pos'       : 28,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'LmacDebugLogs' : {
                'pos'       : 29,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'SjcTraceLogs' : {
                'pos'       : 30,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'PowerControlTraceLogs' : {
                'pos'       : 31,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'BerControl' : {
        'address'    : 0x0338,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'NumBits' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
            'NumPackets' : {
                'pos'       : 16,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'BerMode' : {
        'address'    : 0x033C,
        'length'     : 0x0001,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'DelOnlyMode' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'HpfOverrideSettings' : {
        'address'    : 0x0344,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'HpfMode' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint8_t',
                'enums' : {
                    'Uninitialized'              : 0,
                    'Bypass'                     : 1,
                    'FctTestMode'                : 2,
                    'LbtTestMode'                : 3,
                    '2000Ohm'                    : 4,
                    '500Ohm'                     : 5,
                    'FctTestMode_2'              : 6,
                },
            },
        },
    },
    'AuxAdcControl' : {
        'address'    : 0x0400,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'ChannelEnableBits' : {
                'pos'       :  0,
                'bits'      : 15,
                'resolve_as': 'enum',
                'enums' : {
                    'None'                       : 0,
                    'PowerLo0'                   : 1,
                    'PowerLo1'                   : 2,
                    'PowerLo2'                   : 4,
                    'PowerLo3'                   : 8,
                    'PowerRx0'                   : 16,
                    'PowerRx1'                   : 32,
                    'PowerRx2'                   : 64,
                    'PowerRx3'                   : 128,
                    'TestMux0'                   : 256,
                    'TestMux1'                   : 512,
                    'TestMux2'                   : 1024,
                    'TestMux3'                   : 2048,
                    'Temperature'                : 4096,
                    'PowerLoSum'                 : 8192,
                    'PowerRxSum'                 : 16384,
                },
            },
        },
    },
    'AuxAdcResults' : {
        'address'    : 0x0404,
        'length'     : 0x0002,
        'num_entries': 15,
        'access'     : 'read-only',
        'fields'  : {
            'AdcResult' : {
                'pos'       :  0,
                'bits'      : 10,
                'resolve_as': 'uint16_t',
                'enums' : {
                    'PowerLo0'                   : 0,
                    'PowerLo1'                   : 1,
                    'PowerLo2'                   : 2,
                    'PowerLo3'                   : 3,
                    'PowerRx0'                   : 4,
                    'PowerRx1'                   : 5,
                    'PowerRx2'                   : 6,
                    'PowerRx3'                   : 7,
                    'TestMux0'                   : 8,
                    'TestMux1'                   : 9,
                    'TestMux2'                   : 10,
                    'TestMux3'                   : 11,
                    'Temperature'                : 12,
                    'PowerLoSum'                 : 13,
                    'PowerRxSum'                 : 14,
                },
            },
        },
    },
    'AuxDacControl' : {
        'address'    : 0x0430,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'ChannelEnableBits' : {
                'pos'       :  0,
                'bits'      :  2,
                'resolve_as': 'uint',
            },
        },
    },
    'AuxDacSettings' : {
        'address'    : 0x0432,
        'length'     : 0x0002,
        'num_entries': 2,
        'access'     : 'read-write',
        'fields'  : {
            'Value' : {
                'pos'       :  0,
                'bits'      : 10,
                'resolve_as': 'uint',
            },
        },
    },
    'ATestMux' : {
        'address'    : 0x0440,
        'length'     : 0x0004,
        'num_entries': 4,
        'access'     : 'read-write',
        'fields'  : {
            'ATestMux' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'TxFineGain' : {
        'address'    : 0x0504,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'TxScalar' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'int',
            },
        },
    },
    'RxGainControl' : {
        'address'    : 0x0508,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'RxAtten' : {
                'pos'       :  0,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Atten_0_dB'                 : 0,
                    'Atten_3_dB'                 : 1,
                    'Atten_6_dB'                 : 2,
                    'Atten_12_dB'                : 3,
                },
            },
            'Pga1Gain' : {
                'pos'       :  2,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Gain_n6_dB'                 : 0,
                    'Gain_0_dB'                  : 1,
                    'Gain_6_dB'                  : 2,
                    'Gain_12_dB'                 : 3,
                },
            },
            'Pga2Gain' : {
                'pos'       :  4,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Gain_0_dB'                  : 0,
                    'Gain_6_dB'                  : 1,
                    'Gain_12_dB'                 : 2,
                    'Gain_18_dB'                 : 3,
                },
            },
            'Pga3Gain' : {
                'pos'       :  6,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Gain_0_dB'                  : 0,
                    'Gain_6_dB'                  : 1,
                    'Gain_12_dB'                 : 2,
                    'Gain_18_dB'                 : 3,
                },
            },
            'MixerGain' : {
                'pos'       : 10,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Gain_1p6_dB'                : 0,
                    'Gain_11p2_dB'               : 1,
                    'Gain_17p2_dB'               : 2,
                    'Gain_20p7_dB'               : 3,
                },
            },
            'Pga1RinSelect' : {
                'pos'       : 12,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'MixerBandwidth' : {
                'pos'       : 14,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'TxCoarseGain' : {
        'address'    : 0x050C,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'TxAtten' : {
                'pos'       :  0,
                'bits'      :  5,
                'resolve_as': 'uint',
            },
        },
    },
    'RfMode' : {
        'address'    : 0x0514,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Id' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'DcOffset' : {
        'address'    : 0x0518,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Offset' : {
                'pos'       :  0,
                'bits'      : 20,
                'resolve_as': 'int',
            },
        },
    },
    'EtsiBurstOffTime' : {
        'address'    : 0x051C,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'OffTime' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'CwIsOn' : {
        'address'    : 0x0520,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'IsOn' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'MeasureRssiCount' : {
        'address'    : 0x0528,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Samples' : {
                'pos'       :  0,
                'bits'      :  4,
                'resolve_as': 'uint',
            },
        },
    },
    'MeasuredRssiLinear' : {
        'address'    : 0x0540,
        'length'     : 0x0004,
        'num_entries': 5,
        'access'     : 'read-only',
        'fields'  : {
            'Value' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint',
            },
        },
    },
    'MeasuredRssiLog2' : {
        'address'    : 0x0554,
        'length'     : 0x0002,
        'num_entries': 5,
        'access'     : 'read-only',
        'fields'  : {
            'Value' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'LbtOffset' : {
        'address'    : 0x0560,
        'length'     : 0x0004,
        'num_entries': 5,
        'access'     : 'read-write',
        'fields'  : {
            'Khz' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'int',
            },
        },
    },
    'LbtControl' : {
        'address'    : 0x0574,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Override' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'NarrowBandwidthMode' : {
                'pos'       :  1,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'NumRssiMeasurements' : {
                'pos'       :  8,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'MeasurementDelayUs' : {
                'pos'       : 16,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'RfSynthesizerControl' : {
        'address'    : 0x0588,
        'length'     : 0x0004,
        'num_entries': 5,
        'access'     : 'read-write',
        'fields'  : {
            'NDivider' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
            'RDivider' : {
                'pos'       : 16,
                'bits'      :  3,
                'resolve_as': 'uint',
            },
            'LfType' : {
                'pos'       : 24,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'SjcControl' : {
        'address'    : 0x0600,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'SampleAverageCoarse' : {
                'pos'       :  0,
                'bits'      :  4,
                'resolve_as': 'uint',
            },
            'SampleAverageFine' : {
                'pos'       :  4,
                'bits'      :  4,
                'resolve_as': 'uint',
            },
            'EventsEnable' : {
                'pos'       :  8,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'FixedRxAtten' : {
                'pos'       :  9,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Decimator' : {
                'pos'       : 10,
                'bits'      :  3,
                'resolve_as': 'uint',
            },
        },
    },
    'SjcGainControl' : {
        'address'    : 0x0604,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'RxAtten' : {
                'pos'       :  0,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Atten_0_dB'                 : 0,
                    'Atten_3_dB'                 : 1,
                    'Atten_6_dB'                 : 2,
                    'Atten_12_dB'                : 3,
                },
            },
            'Pga1Gain' : {
                'pos'       :  2,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Gain_n6_dB'                 : 0,
                    'Gain_0_dB'                  : 1,
                    'Gain_6_dB'                  : 2,
                    'Gain_12_dB'                 : 3,
                },
            },
            'Pga2Gain' : {
                'pos'       :  4,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Gain_0_dB'                  : 0,
                    'Gain_6_dB'                  : 1,
                    'Gain_12_dB'                 : 2,
                    'Gain_18_dB'                 : 3,
                },
            },
            'Pga3Gain' : {
                'pos'       :  6,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Gain_0_dB'                  : 0,
                    'Gain_6_dB'                  : 1,
                    'Gain_12_dB'                 : 2,
                    'Gain_18_dB'                 : 3,
                },
            },
            'MixerGain' : {
                'pos'       : 10,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'Gain_1p6_dB'                : 0,
                    'Gain_11p2_dB'               : 1,
                    'Gain_17p2_dB'               : 2,
                    'Gain_20p7_dB'               : 3,
                },
            },
            'Pga1RinSelect' : {
                'pos'       : 12,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'MixerBandwidth' : {
                'pos'       : 14,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'SjcInitialSettlingTime' : {
        'address'    : 0x0608,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'SettlingTime' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'SjcResidueSettlingTime' : {
        'address'    : 0x060C,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'SettlingTime' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'SjcCdacI' : {
        'address'    : 0x0610,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Center' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'int',
            },
            'Limit' : {
                'pos'       :  8,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'StepSize' : {
                'pos'       : 16,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
        },
    },
    'SjcCdacQ' : {
        'address'    : 0x0614,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Center' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'int',
            },
            'Limit' : {
                'pos'       :  8,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'StepSize' : {
                'pos'       : 16,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
        },
    },
    'SjcResultI' : {
        'address'    : 0x0618,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'Cdac' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'int',
            },
            'CdacSkuLimited' : {
                'pos'       :  8,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Residue' : {
                'pos'       : 12,
                'bits'      : 20,
                'resolve_as': 'int',
            },
        },
    },
    'SjcResultQ' : {
        'address'    : 0x061C,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-only',
        'fields'  : {
            'Cdac' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'int',
            },
            'CdacSkuLimited' : {
                'pos'       :  8,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Residue' : {
                'pos'       : 12,
                'bits'      : 20,
                'resolve_as': 'int',
            },
        },
    },
    'SjcResidueThreshold' : {
        'address'    : 0x0620,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'magnitude' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'AnalogEnable' : {
        'address'    : 0x0700,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'All' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'AggregateOpBuffer' : {
        'address'    : 0x0704,
        'length'     : 0x0200,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'CommandBuffer' : {
                'pos'       :  0,
                'bits'      : 4096,
                'resolve_as': 'bytes',
            },
        },
    },
    'PowerDroopCompensation' : {
        'address'    : 0x0904,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Enable' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'rfu' : {
                'pos'       :  1,
                'bits'      : 15,
                'resolve_as': 'uint16_t',
            },
            'CompensationIntervalMs' : {
                'pos'       : 16,
                'bits'      :  8,
                'resolve_as': 'uint8_t',
            },
            'FineGainStepCdB' : {
                'pos'       : 24,
                'bits'      :  8,
                'resolve_as': 'uint8_t',
            },
        },
    },
    'LastTxRampUpTimeMs' : {
        'address'    : 0x0908,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'TimeMs' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint32_t',
            },
        },
    },
    'LastTxRampUpLoFreqKhz' : {
        'address'    : 0x090C,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'LoFreqKhz' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint32_t',
            },
        },
    },
    'LastTxRampDownTimeMs' : {
        'address'    : 0x0910,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'TimeMs' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint32_t',
            },
        },
    },
    'LastTxRampDownLoFreqKhz' : {
        'address'    : 0x0914,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'LoFreqKhz' : {
                'pos'       :  0,
                'bits'      : 32,
                'resolve_as': 'uint32_t',
            },
        },
    },
    'LastTxRampDownReason' : {
        'address'    : 0x0918,
        'length'     : 0x0001,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Reason' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint8_t',
            },
        },
    },
    'RssiThresholdRn16' : {
        'address'    : 0x0FFC,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Threshold' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'RssiThresholdEpc' : {
        'address'    : 0x0FFE,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'Threshold' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'InventoryRoundControl' : {
        'address'    : 0x1000,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'InitialQ' : {
                'pos'       :  0,
                'bits'      :  4,
                'resolve_as': 'uint',
            },
            'MaxQ' : {
                'pos'       :  4,
                'bits'      :  4,
                'resolve_as': 'uint',
            },
            'MinQ' : {
                'pos'       :  8,
                'bits'      :  4,
                'resolve_as': 'uint',
            },
            'NumMinQCycles' : {
                'pos'       : 12,
                'bits'      :  4,
                'resolve_as': 'uint',
            },
            'FixedQMode' : {
                'pos'       : 16,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'QIncreaseUseQuery' : {
                'pos'       : 17,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'QDecreaseUseQuery' : {
                'pos'       : 18,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'Session' : {
                'pos'       : 19,
                'bits'      :  2,
                'resolve_as': 'enum',
                'enums' : {
                    'S0'                         : 0,
                    'S1'                         : 1,
                    'S2'                         : 2,
                    'S3'                         : 3,
                },
            },
            'Select' : {
                'pos'       : 21,
                'bits'      :  2,
                'resolve_as': 'uint',
            },
            'Target' : {
                'pos'       : 23,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'HaltOnAllTags' : {
                'pos'       : 24,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'FastIdEnable' : {
                'pos'       : 25,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'TagFocusEnable' : {
                'pos'       : 26,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AutoAccess' : {
                'pos'       : 27,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AbortOnFail' : {
                'pos'       : 28,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'HaltOnFail' : {
                'pos'       : 29,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AlwaysAck' : {
                'pos'       : 30,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'InventoryRoundControl_2' : {
        'address'    : 0x1004,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'MaxQueriesSinceValidEpc' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'StartingMinQCount' : {
                'pos'       : 16,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'StartingMaxQueriesSinceValidEpcCount' : {
                'pos'       : 24,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
        },
    },
    'NominalStopTime' : {
        'address'    : 0x1008,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'DwellTime' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'ExtendedStopTime' : {
        'address'    : 0x100C,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'DwellTime' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'RegulatoryStopTime' : {
        'address'    : 0x1010,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'DwellTime' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'Gen2SelectEnable' : {
        'address'    : 0x1014,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'SelectEnables' : {
                'pos'       :  0,
                'bits'      : 10,
                'resolve_as': 'uint',
            },
        },
    },
    'Gen2AccessEnable' : {
        'address'    : 0x1018,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'AccessEnables' : {
                'pos'       :  0,
                'bits'      : 10,
                'resolve_as': 'uint',
            },
        },
    },
    'Gen2AutoAccessEnable' : {
        'address'    : 0x101C,
        'length'     : 0x0002,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'AutoAccessEnables' : {
                'pos'       :  0,
                'bits'      : 10,
                'resolve_as': 'uint',
            },
        },
    },
    'Gen2Offsets' : {
        'address'    : 0x1020,
        'length'     : 0x0001,
        'num_entries': 10,
        'access'     : 'read-write',
        'fields'  : {
            'Offset' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint8_t',
            },
        },
    },
    'Gen2Lengths' : {
        'address'    : 0x1030,
        'length'     : 0x0002,
        'num_entries': 10,
        'access'     : 'read-write',
        'fields'  : {
            'Length' : {
                'pos'       :  0,
                'bits'      : 16,
                'resolve_as': 'uint16_t',
            },
        },
    },
    'Gen2TransactionIds' : {
        'address'    : 0x1050,
        'length'     : 0x0001,
        'num_entries': 10,
        'access'     : 'read-write',
        'fields'  : {
            'TransactionId' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint8_t',
            },
        },
    },
    'Gen2TxnControls' : {
        'address'    : 0x1060,
        'length'     : 0x0004,
        'num_entries': 10,
        'access'     : 'read-write',
        'fields'  : {
            'ResponseType' : {
                'pos'       :  0,
                'bits'      :  3,
                'resolve_as': 'uint',
            },
            'HasHeaderBit' : {
                'pos'       :  3,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'UseCoverCode' : {
                'pos'       :  4,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AppendHandle' : {
                'pos'       :  5,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'AppendCrc16' : {
                'pos'       :  6,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'IsKillCommand' : {
                'pos'       :  7,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
            'RxLength' : {
                'pos'       : 16,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'DropQueryControl' : {
        'address'    : 0x1090,
        'length'     : 0x0004,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'DropPower' : {
                'pos'       :  0,
                'bits'      :  8,
                'resolve_as': 'uint',
            },
            'DropDwell' : {
                'pos'       : 16,
                'bits'      : 16,
                'resolve_as': 'uint',
            },
        },
    },
    'TagFeaturesControl' : {
        'address'    : 0x1094,
        'length'     : 0x0001,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'MemoryDiagnosticMode' : {
                'pos'       :  0,
                'bits'      :  1,
                'resolve_as': 'bool',
            },
        },
    },
    'Gen2TxBuffer' : {
        'address'    : 0x1100,
        'length'     : 0x0080,
        'num_entries': 1,
        'access'     : 'read-write',
        'fields'  : {
            'TxBuffer' : {
                'pos'       :  0,
                'bits'      : 1024,
                'resolve_as': 'bytes',
            },
        },
    },
    'CalibrationInfo' : {
        'address'    : 0xE800,
        'length'     : 0x0800,
        'num_entries': 1,
        'access'     : 'read-only',
    },
}
# IPJ_autogen }
