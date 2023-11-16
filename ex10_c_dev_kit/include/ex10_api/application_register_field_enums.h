/*****************************************************************************
 *                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      *
 *                                                                           *
 * This source code is the property of Impinj, Inc. Your use of this source  *
 * code in whole or in part is subject to your applicable license terms      *
 * from Impinj.                                                              *
 * Contact support@impinj.com for a copy of the applicable Impinj license    *
 * terms.                                                                    *
 *                                                                           *
 * (c) Copyright 2020 - 2023 Impinj, Inc. All rights reserved.               *
 *                                                                           *
 *****************************************************************************/
// enums used by the register fields

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumerate the inventoried flag A or B to indicate which inventory round
 * a tag will participate in. Use to set InventoryRoundControlFields.target
 */
enum
{
    target_A = 0u,
    target_B = 1u,
};

// clang-format off
// IPJ_autogen | gen_c_app_ex10_api_reg_field_enums {

#define MaxNumSelects            ((size_t)10)
#define MaxNumGen2Commands       ((size_t)10)
#define MaxNumRssiMeasurements   ((size_t)5)
#define AuxAdcChannelCount       ((size_t)15)
#define AuxDacChannelCount       ((size_t)2)
#define ATestMuxChannelCount     ((size_t)4)

enum ResponseCode {
    Success                      = 0xa5,
    CommandInvalid               = 0x01,
    ArgumentInvalid              = 0x02,
    ResponseOverflow             = 0x06,
    CommandMalformed             = 0x07,
    AddressWriteFailure          = 0x08,
    ImageInvalid                 = 0x09,
    LengthInvalid                = 0x0a,
    UploadStateInvalid           = 0x0b,
    BadCrc                       = 0x0e,
    FlashInvalidPage             = 0x0f,
    FlashPageLocked              = 0x10,
    FlashEraseFailure            = 0x11,
    FlashProgramFailure          = 0x12,
    StoredSettingsMalformed      = 0x13,
    NotEnoughSpace               = 0x14,
};

enum CommandCode {
    CommandRead                  = 0x01,
    CommandWrite                 = 0x02,
    CommandReadFifo              = 0x03,
    CommandStartUpload           = 0x04,
    CommandContinueUpload        = 0x05,
    CommandCompleteUpload        = 0x06,
    CommandReValidateMainImage   = 0x07,
    CommandReset                 = 0x08,
    CommandTestTransfer          = 0x0a,
    CommandWriteInfoPage         = 0x0b,
    CommandTestRead              = 0x0c,
    CommandInsertFifoEvent       = 0x0e,
};

enum Status {
    Bootloader                   = 0x01,
    Application                  = 0x02,
};

enum OpId {
    Idle                         = 0xa0,
    LogTestOp                    = 0xa1,
    MeasureAdcOp                 = 0xa2,
    TxRampUpOp                   = 0xa3,
    TxRampDownOp                 = 0xa4,
    SetTxCoarseGainOp            = 0xa5,
    SetTxFineGainOp              = 0xa6,
    RadioPowerControlOp          = 0xa7,
    SetRfModeOp                  = 0xa8,
    SetRxGainOp                  = 0xa9,
    LockSynthesizerOp            = 0xaa,
    EventFifoTestOp              = 0xab,
    RxRunSjcOp                   = 0xac,
    SetGpioOp                    = 0xad,
    SetClearGpioPinsOp           = 0xae,
    StartInventoryRoundOp        = 0xb0,
    RunPrbsDataOp                = 0xb1,
    SendSelectOp                 = 0xb2,
    SetDacOp                     = 0xb3,
    SetATestMuxOp                = 0xb4,
    PowerControlLoopOp           = 0xb5,
    MeasureRssiOp                = 0xb6,
    UsTimerStartOp               = 0xb7,
    UsTimerWaitOp                = 0xb8,
    AggregateOp                  = 0xb9,
    ListenBeforeTalkOp           = 0xba,
    BerTestOp                    = 0xc0,
    EtsiBurstOp                  = 0xc1,
    HpfOverrideTestOp            = 0xc2,
    SetDcOffsetOp                = 0xc4,
};

enum OpsStatus {
    ErrorNone                     = 0x00,
    ErrorUnknownOp                = 0x01,
    ErrorUnknownError             = 0x02,
    ErrorInvalidParameter         = 0x03,
    ErrorPllNotLocked             = 0x04,
    ErrorPowerControlTargetFailed = 0x05,
    ErrorInvalidTxState           = 0x06,
    ErrorRadioPowerNotEnabled     = 0x07,
    ErrorAggregateBufferOverflow  = 0x08,
    ErrorAggregateInnerOpError    = 0x09,
    ErrorSjcCdacRangeError        = 0x0b,
    ErrorSjcResidueThresholdExceeded = 0x0c,
    ErrorDroopCompensationTooManyAdcChannels = 0x0d,
    ErrorEventFailedToSend        = 0x0e,
    ErrorAggregateEx10CommandError = 0x0f,
    ErrorUnsupportedCommand       = 0x10,
    ErrorBerRxHung                = 0x11,
};

enum HaltedStatusError {
    ErrorNoError                 = 0x00,
    ErrorCoverCodeSizeError      = 0x01,
    ErrorGetCoverCodeFailed      = 0x02,
    ErrorBadCrc                  = 0x03,
    ErrorUnknown                 = 0x04,
};

enum HpfOverrideSettingsHpfMode {
    HpfModeUninitialized         = 0x00,
    HpfModeBypass                = 0x01,
    HpfModeFctTestMode           = 0x02,
    HpfModeLbtTestMode           = 0x03,
    HpfMode2000Ohm               = 0x04,
    HpfMode500Ohm                = 0x05,
    HpfModeFctTestMode_2         = 0x06,
};

enum AuxAdcControlChannelEnableBits {
    ChannelEnableBitsNone        = 0x00,
    ChannelEnableBitsPowerLo0    = 0x01,
    ChannelEnableBitsPowerLo1    = 0x02,
    ChannelEnableBitsPowerLo2    = 0x04,
    ChannelEnableBitsPowerLo3    = 0x08,
    ChannelEnableBitsPowerRx0    = 0x10,
    ChannelEnableBitsPowerRx1    = 0x20,
    ChannelEnableBitsPowerRx2    = 0x40,
    ChannelEnableBitsPowerRx3    = 0x80,
    ChannelEnableBitsTestMux0    = 0x100,
    ChannelEnableBitsTestMux1    = 0x200,
    ChannelEnableBitsTestMux2    = 0x400,
    ChannelEnableBitsTestMux3    = 0x800,
    ChannelEnableBitsTemperature = 0x1000,
    ChannelEnableBitsPowerLoSum  = 0x2000,
    ChannelEnableBitsPowerRxSum  = 0x4000,
};

enum AuxAdcResultsAdcResult {
    AdcResultPowerLo0            = 0x00,
    AdcResultPowerLo1            = 0x01,
    AdcResultPowerLo2            = 0x02,
    AdcResultPowerLo3            = 0x03,
    AdcResultPowerRx0            = 0x04,
    AdcResultPowerRx1            = 0x05,
    AdcResultPowerRx2            = 0x06,
    AdcResultPowerRx3            = 0x07,
    AdcResultTestMux0            = 0x08,
    AdcResultTestMux1            = 0x09,
    AdcResultTestMux2            = 0x0a,
    AdcResultTestMux3            = 0x0b,
    AdcResultTemperature         = 0x0c,
    AdcResultPowerLoSum          = 0x0d,
    AdcResultPowerRxSum          = 0x0e,
};

enum RxGainControlRxAtten {
    RxAttenAtten_0_dB            = 0x00,
    RxAttenAtten_3_dB            = 0x01,
    RxAttenAtten_6_dB            = 0x02,
    RxAttenAtten_12_dB           = 0x03,
};

enum RxGainControlPga1Gain {
    Pga1GainGain_n6_dB           = 0x00,
    Pga1GainGain_0_dB            = 0x01,
    Pga1GainGain_6_dB            = 0x02,
    Pga1GainGain_12_dB           = 0x03,
};

enum RxGainControlPga2Gain {
    Pga2GainGain_0_dB            = 0x00,
    Pga2GainGain_6_dB            = 0x01,
    Pga2GainGain_12_dB           = 0x02,
    Pga2GainGain_18_dB           = 0x03,
};

enum RxGainControlPga3Gain {
    Pga3GainGain_0_dB            = 0x00,
    Pga3GainGain_6_dB            = 0x01,
    Pga3GainGain_12_dB           = 0x02,
    Pga3GainGain_18_dB           = 0x03,
};

enum RxGainControlMixerGain {
    MixerGainGain_1p6_dB         = 0x00,
    MixerGainGain_11p2_dB        = 0x01,
    MixerGainGain_17p2_dB        = 0x02,
    MixerGainGain_20p7_dB        = 0x03,
};

enum InventoryRoundControlSession {
    SessionS0                    = 0x00,
    SessionS1                    = 0x01,
    SessionS2                    = 0x02,
    SessionS3                    = 0x03,
};
// IPJ_autogen }

enum ProductSku {
    SkuUnknown = 0x0,
    SkuE310    = 0x0310,
    SkuE510    = 0x0510,
    SkuE710    = 0x0710,
    SkuE910    = 0x0910,
};

// clang-format on

#ifdef __cplusplus
}
#endif
