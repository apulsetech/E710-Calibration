#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2020 - 2021 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################

from enum import IntEnum


# IPJ_autogen | gen_c2python_enums {

class ResponseCode(IntEnum):
    Success                      = 0xA5
    CommandInvalid               = 0x01
    ArgumentInvalid              = 0x02
    ResponseOverflow             = 0x06
    CommandMalformed             = 0x07
    AddressWriteFailure          = 0x08
    ImageInvalid                 = 0x09
    LengthInvalid                = 0x0A
    UploadStateInvalid           = 0x0B
    BadCrc                       = 0x0E
    FlashInvalidPage             = 0x0F
    FlashPageLocked              = 0x10
    FlashEraseFailure            = 0x11
    FlashProgramFailure          = 0x12
    StoredSettingsMalformed      = 0x13
    NotEnoughSpace               = 0x14

class CommandCode(IntEnum):
    CommandRead                  = 0x01
    CommandWrite                 = 0x02
    CommandReadFifo              = 0x03
    CommandStartUpload           = 0x04
    CommandContinueUpload        = 0x05
    CommandCompleteUpload        = 0x06
    CommandReValidateMainImage   = 0x07
    CommandReset                 = 0x08
    CommandTestTransfer          = 0x0A
    CommandWriteInfoPage         = 0x0B
    CommandTestRead              = 0x0C
    CommandInsertFifoEvent       = 0x0E

class Status(IntEnum):
    Bootloader                   = 0x01
    Application                  = 0x02

class OpId(IntEnum):
    Idle                         = 0xA0
    LogTestOp                    = 0xA1
    MeasureAdcOp                 = 0xA2
    TxRampUpOp                   = 0xA3
    TxRampDownOp                 = 0xA4
    SetTxCoarseGainOp            = 0xA5
    SetTxFineGainOp              = 0xA6
    RadioPowerControlOp          = 0xA7
    SetRfModeOp                  = 0xA8
    SetRxGainOp                  = 0xA9
    LockSynthesizerOp            = 0xAA
    EventFifoTestOp              = 0xAB
    RxRunSjcOp                   = 0xAC
    SetGpioOp                    = 0xAD
    SetClearGpioPinsOp           = 0xAE
    StartInventoryRoundOp        = 0xB0
    RunPrbsDataOp                = 0xB1
    SendSelectOp                 = 0xB2
    SetDacOp                     = 0xB3
    SetATestMuxOp                = 0xB4
    PowerControlLoopOp           = 0xB5
    MeasureRssiOp                = 0xB6
    UsTimerStartOp               = 0xB7
    UsTimerWaitOp                = 0xB8
    AggregateOp                  = 0xB9
    ListenBeforeTalkOp           = 0xBA
    BerTestOp                    = 0xC0
    EtsiBurstOp                  = 0xC1
    HpfOverrideTestOp            = 0xC2
    SetDcOffsetOp                = 0xC4

class OpsStatus(IntEnum):
    ErrorNone                     = 0x00
    ErrorUnknownOp                = 0x01
    ErrorUnknownError             = 0x02
    ErrorInvalidParameter         = 0x03
    ErrorPllNotLocked             = 0x04
    ErrorPowerControlTargetFailed = 0x05
    ErrorInvalidTxState           = 0x06
    ErrorRadioPowerNotEnabled     = 0x07
    ErrorAggregateBufferOverflow  = 0x08
    ErrorAggregateInnerOpError    = 0x09
    ErrorSjcCdacRangeError        = 0x0B
    ErrorSjcResidueThresholdExceeded = 0x0C
    ErrorDroopCompensationTooManyAdcChannels = 0x0D
    ErrorEventFailedToSend        = 0x0E
    ErrorAggregateEx10CommandError = 0x0F
    ErrorUnsupportedCommand       = 0x10
    ErrorBerRxHung                = 0x11

class HaltedStatusError(IntEnum):
    ErrorNoError                 = 0x00
    ErrorCoverCodeSizeError      = 0x01
    ErrorGetCoverCodeFailed      = 0x02
    ErrorBadCrc                  = 0x03
    ErrorUnknown                 = 0x04

class HpfOverrideSettingsHpfMode(IntEnum):
    HpfModeUninitialized         = 0x00
    HpfModeBypass                = 0x01
    HpfModeFctTestMode           = 0x02
    HpfModeLbtTestMode           = 0x03
    HpfMode2000Ohm               = 0x04
    HpfMode500Ohm                = 0x05
    HpfModeFctTestMode_2         = 0x06

class AuxAdcControlChannelEnableBits(IntEnum):
    ChannelEnableBitsNone        = 0x00
    ChannelEnableBitsPowerLo0    = 0x01
    ChannelEnableBitsPowerLo1    = 0x02
    ChannelEnableBitsPowerLo2    = 0x04
    ChannelEnableBitsPowerLo3    = 0x08
    ChannelEnableBitsPowerRx0    = 0x10
    ChannelEnableBitsPowerRx1    = 0x20
    ChannelEnableBitsPowerRx2    = 0x40
    ChannelEnableBitsPowerRx3    = 0x80
    ChannelEnableBitsTestMux0    = 0x100
    ChannelEnableBitsTestMux1    = 0x200
    ChannelEnableBitsTestMux2    = 0x400
    ChannelEnableBitsTestMux3    = 0x800
    ChannelEnableBitsTemperature = 0x1000
    ChannelEnableBitsPowerLoSum  = 0x2000
    ChannelEnableBitsPowerRxSum  = 0x4000

class AuxAdcResultsAdcResult(IntEnum):
    AdcResultPowerLo0            = 0x00
    AdcResultPowerLo1            = 0x01
    AdcResultPowerLo2            = 0x02
    AdcResultPowerLo3            = 0x03
    AdcResultPowerRx0            = 0x04
    AdcResultPowerRx1            = 0x05
    AdcResultPowerRx2            = 0x06
    AdcResultPowerRx3            = 0x07
    AdcResultTestMux0            = 0x08
    AdcResultTestMux1            = 0x09
    AdcResultTestMux2            = 0x0A
    AdcResultTestMux3            = 0x0B
    AdcResultTemperature         = 0x0C
    AdcResultPowerLoSum          = 0x0D
    AdcResultPowerRxSum          = 0x0E

class RxGainControlRxAtten(IntEnum):
    RxAttenAtten_0_dB            = 0x00
    RxAttenAtten_3_dB            = 0x01
    RxAttenAtten_6_dB            = 0x02
    RxAttenAtten_12_dB           = 0x03

class RxGainControlPga1Gain(IntEnum):
    Pga1GainGain_n6_dB           = 0x00
    Pga1GainGain_0_dB            = 0x01
    Pga1GainGain_6_dB            = 0x02
    Pga1GainGain_12_dB           = 0x03

class RxGainControlPga2Gain(IntEnum):
    Pga2GainGain_0_dB            = 0x00
    Pga2GainGain_6_dB            = 0x01
    Pga2GainGain_12_dB           = 0x02
    Pga2GainGain_18_dB           = 0x03

class RxGainControlPga3Gain(IntEnum):
    Pga3GainGain_0_dB            = 0x00
    Pga3GainGain_6_dB            = 0x01
    Pga3GainGain_12_dB           = 0x02
    Pga3GainGain_18_dB           = 0x03

class RxGainControlMixerGain(IntEnum):
    MixerGainGain_1p6_dB         = 0x00
    MixerGainGain_11p2_dB        = 0x01
    MixerGainGain_17p2_dB        = 0x02
    MixerGainGain_20p7_dB        = 0x03

class InventoryRoundControlSession(IntEnum):
    SessionS0                    = 0x00
    SessionS1                    = 0x01
    SessionS2                    = 0x02
    SessionS3                    = 0x03
# IPJ_autogen }
