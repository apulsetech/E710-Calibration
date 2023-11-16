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

#pragma once

#include "board/driver_list.h"
#include "ex10_api/application_registers.h"
#include "ex10_api/bootloader_registers.h"
#include "ex10_api/commands.h"
#include "ex10_api/crc16.h"
#include "ex10_api/ex10_result.h"
#include "ex10_api/fifo_buffer_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/// The maximum size of the application image in bytes.
#define EX10_MAX_IMAGE_BYTES ((size_t)(254000u))

/// The size of a FLASH INFO PAGE on the Impinj Reader Chip.
/// Independent Flash Info pages are dedicated to holding calibration data
/// and stored settings data. @see enum PageIds
#define EX10_INFO_PAGE_SIZE ((size_t)(2048u))

enum
{
    UploadFlash = 0x01
};

/**
 * @enum PageIds
 * Each FLASH region, enumerated by type.
 */
enum PageIds
{
    MainBlockId       = 0u,  ///< The application image.
    FeatureControlsId = 1u,
    ManufacturingId   = 2u,
    CalPageId         = 3u,  ///< The INFO PAGE containing calibration.
    StoredSettingsId  = 4u,  ///< The stored settings data.
};

/**
 * @struct Ex10FirmwareVersion
 * Each Impinj Reader Chip firmware version can be expressed with the fields
 * of this structure.
 */
struct Ex10FirmwareVersion
{
    /// An ASCII encoded string containing the major.minor.patch information.
    char version_string[VERSION_STRING_REG_LENGTH];

    /// The first (left-most) git hash bytes of the firmware image.
    uint8_t git_hash_bytes[GIT_HASH_REG_LENGTH];

    /// The build number of the released firmware image.
    uint32_t build_number;
};

/**
 * @struct Ex10Protocol
 * Ex10 Protocol interface.
 */
struct Ex10Protocol
{
    /**
     * Initialize the Ex10Protocol object.
     * This sets up access to an Impinj Reader Chip.
     *
     * @note This function does not perform any interaction with the
     *       Impinj Reader Chip; it only sets up the object.
     *
     * @param driver_list The interface to use to communicate with the Ex10.
     */
    void (*init)(struct Ex10DriverList const* driver_list);

    /**
     * Initialize the Impinj Reader Chip at the Ex10Protocol level.
     * - Sets the default EventFifo threshold level to 2048 bytes.
     * - Clears the Ex10 interrupt mask register.
     * - Clears the interrupt status register by reading it.
     *
     * @note The Ex10 must be powered on prior to calling this function.
     *        The chain of Ex10 module processing is enabled in this call.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *         If a pthread related initialization error occurred during
     *         initialization, the 'sdk_result_code' will be set to
     *         'Ex10SdkErrorGpio' and the POSIX error code will be passed
     *         under 'device_status' field of the result.
     */
    struct Ex10Result (*init_ex10)(void);

    /**
     * Release any resources used by the Ex10Protocol object.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*deinit)(void);

    /**
     * Register an optional callback for EventFifo data events.
     * @param fifo_cb This callback is triggered when the interrupt_cb
     *                registered through register_interrupt_callback()
     *                returns true.
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     * @see register_interrupt_callback
     */
    struct Ex10Result (*register_fifo_data_callback)(
        void (*fifo_cb)(struct FifoBufferNode*));

    /**
     * Enable specified interrupts and register a callback
     * @param enable_mask The mask of interrupts to enable.
     * @param interrupt_cb A function called when an enabled interrupt fires.
     * The callback receives the interrupt status bits via argument and it
     * should return true or false. If the return value is true then the
     * fifo_data callback is triggered. @see register_fifo_data_callback.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*register_interrupt_callback)(
        struct InterruptMaskFields enable_mask,
        bool (*interrupt_cb)(struct InterruptStatusFields));

    /// Unregister the callback used to deal with fifo data
    void (*unregister_fifo_data_callback)(void);

    /**
     * Unregister the callback for interrupts and write to the Ex10 device to
     * disable the interrupts.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*unregister_interrupt_callback)(void);

    /**
     * Enable or disable Impinj Reader Chip interrupt processing.
     *
     * @attention Do not call this function from the IRQ_N monitor thread
     *            context. A deadlock will occur.
     *
     * @param enable If true, the interrupt and fifo data handlers
     *               will be called when the Impinj Reader Chip triggers the
     *               IRQ_N monitor thread. If false, then all interrupts are
     *               ignored from the IRQ_N monitor thread.
     */
    void (*enable_interrupt_handlers)(bool enable);

    /**
     * Read an Ex10 Register.
     *
     * @param reg_info A description of the register to access.
     * @param buffer The buffer data from the register will be read into.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*read)(struct RegisterInfo const* const reg_info,
                              void*                            buffer);

    /**
     * Read raw data from the Impinj Reader Chip internal memory layout.
     *
     * @param address Address to begin reading from
     * @param length  Number of bytes to read from the register.
     * @param buffer  The user supplied data buffer into which data read
     *                from the Ex10 will be written to.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*test_read)(uint32_t address,
                                   uint16_t length,
                                   void*    buffer);

    /**
     * Read an Ex10 Register.
     *
     * @param reg_info A description of the register to access.
     * @param buffer The buffer data from the register will be read into.
     * @param index    Provides an offset into registers which have multiple
     * entries. Every register has a num_entires fields, where some registers
     * are divided into pieces when used. This is documented on a reg by reg
     * basis. If an index exceeding that allowed by the register is passed in,
     * an error is returned.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*read_index)(struct RegisterInfo const* const reg_info,
                                    void*                            buffer,
                                    uint8_t                          index);

    /**
     * Write an Ex10 Register.
     *
     * @param reg_info A description of the register to access.
     * @param buffer   The data to be written to buffer.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*write)(struct RegisterInfo const* const reg_info,
                               void const*                      buffer);

    /**
     * Write an Ex10 Register.
     *
     * @param reg_info A description of the register to access.
     * @param buffer   The data to be written to buffer.
     * @param index    Provides an offset into registers which have multiple
     * entries. Every register has a num_entires fields, where some registers
     * are divided into pieces when used. This is documented on a reg by reg
     * basis. If an index exceeding that allowed by the register is passed in,
     * an error is returned.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*write_index)(struct RegisterInfo const* const reg_info,
                                     void const*                      buffer,
                                     uint8_t                          index);

    /**
     * Read an Ex10 register blob.
     *
     * @param address Address to begin reading from
     * @param length  Number of bytes to read from the register.
     * @param buffer  The buffer data from the register will be written to.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*read_partial)(uint16_t address,
                                      uint16_t length,
                                      void*    buffer);

    /**
     * Write an Ex10 register blob.
     *
     * @param address Address to begin writing to
     * @param length  Number of bytes to write to the register.
     * @param buffer  The data to be written to the register.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*write_partial)(uint16_t    address,
                                       uint16_t    length,
                                       void const* buffer);

    /**
     * Write multiple registers to the Ex10 device.
     *
     * @param regs     A list of registers to write to
     * @param buffers  A list of buffers with data to write to each register in
     * regs
     * @param num_regs The number of entries in regs and buffers.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed..
     */
    struct Ex10Result (*write_multiple)(struct RegisterInfo const* const regs[],
                                        void const* buffers[],
                                        size_t      num_regs);

    /**
     * Get a span containing data written to multiple registers.
     *
     * The span will contain a write command, address, length, and data
     * suitable for stored settings.
     *
     * @param regs_list A list of registers to write.
     * @param buffers   A list of buffers with data to write to each register
     *                  contained within regs_list.
     * @param num_regs  The number of entries in regs and buffers.
     * @param [in/out]  span
     *                  The destination span that will contain all the writes.
     *                  The span.length value specifies the buffer size being
     *                  passed in. This value is modified to indicate the number
     *                  of bytes copied into the span.data buffer.
     *                  The span.data member must point to valid data.
     *                  This function will fill the data buffer; the data member
     *                  value will not be changed.
     * @param [out]     regs_copied
     *                  The number of nodes of reg_list[], buffers[] pairs that
     *                  were copied into the span. If not all nodes fit then
     *                  this return value will be less than the num_regs value
     *                  passed in.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed..
     */
    struct Ex10Result (*get_write_multiple_stored_settings)(
        struct RegisterInfo const* const regs_list[],
        void const*                      buffers[],
        size_t                           num_regs,
        struct ByteSpan*                 span,
        size_t*                          regs_copied);

    /**
     * Read an info page from flash.
     *
     * @param address     The base info page address.
     * @param read_buffer Destination buffer for the page data,
     *                    2048 bytes in size.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*read_info_page_buffer)(uint32_t address,
                                               uint8_t* read_buffer);

    /**
     * Read multiple registers from the Ex10 device.
     *
     * @param regs_list A list of registers to read from.
     * @param buffers   A list of destination buffers for register data.
     * @param num_regs  The number of entries in regs and buffers.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*read_multiple)(
        struct RegisterInfo const* const regs_list[],
        void*                            buffers[],
        size_t                           num_regs);

    /**
     * Command the Ex10 device to start running the specified op.
     *
     * @param op_id The op to run.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*start_op)(enum OpId op_id);

    /**
     * Command the Ex10 device to stop running an ongoing op.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*stop_op)(void);

    /**
     * Check to see if an op is currently active
     *
     * @return bool  true if an op is busy
     */
    bool (*is_op_currently_running)(void);

    /**
     * Block until ongoing op has completed.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*wait_op_completion)(void);

    /**
     * Block until ongoing op has completed.
     *
     * @param timeout_ms Function returns false if the op takes more
     *                   time than this number of milliseconds.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*wait_op_completion_with_timeout)(uint32_t timeout_ms);

    /**
     * Read the ops status register.
     *
     * @param ops_status The structure to fill in the register read from the
     *                   device.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*read_ops_status_reg)(
        struct OpsStatusFields* ops_status);

    /**
     * Issue a soft reset to the Ex10 chip.
     *
     * @param destination The requested execution context of the Ex10 device:
     *        Application or Bootloader.
     *
     * @note Requesting a reset into the Application does not guarantee that
     *       the device will actually enter Application mode.
     *
     * If there is no valid application image loaded into the Ex10, then the
     * request to reset into the Application will result in a reset into the
     * Bootloader and an error will be reported.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *         Errors reported when:
     *         - If the final running location is not the requested execution
     *           context, an error will be reported with result code of
     *           `Ex10SdkErrorRunLocation`.
     *         - If an error occurred during reopenning of the host interface,
     *           will be reported with result code of
     *           `Ex10SdkErrorHostInterface`. In addition, operating system
     *           error code, like negative POSIX ERRNO, for example, will be
     *           included in device status field.
     *         - Any error received from `commands` layer
     */
    struct Ex10Result (*reset)(enum Status destination);

    /**
     * Set the EventFifo threshold in bytes. When the number of bytes
     * accumulated by the Ex10 is this threshold or higher then an interrupt
     * wil be generated.
     *
     * @param threshold The EventFifo threshold in bytes.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*set_event_fifo_threshold)(size_t threshold);

    /**
     * Insert a host defined event in the event fifo stream.
     *
     * @param event_packet The EventFifo packet append to the Ex10 EventFifo.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*insert_fifo_event)(
        const bool                    trigger_irq,
        struct EventFifoPacket const* event_packet);

    /**
     * Get the running location of the FW (Application or Bootloader)
     *
     * @return The running location of the Ex10Device
     */
    enum Status (*get_running_location)(void);

    /**
     * Read the Rx gain parameters from the Ex10 device.
     *
     * @note The field RxGainControlFields.rx_atten is modified when the
     *       RxRunSjcOp is run. To maintain the accuracy of this value,
     *       the rx_atten value should be updated when the sjc_measurement
     *       EventFifo packet is received.
     *
     * @note If this value is read after the RxRunSjcOp is run, then the
     *       rx_atten value will remain valid until the transmitter
     *       is ramped down.
     *
     * @return struct RxGainControlFields The Rx gain parameters.
     */
    struct RxGainControlFields (*get_analog_rx_config)(void);

    /**
     * Allows writing to device flash pages.
     * A CRC-16-CCITT will be calculated across the data and appended to the
     * written data.
     *
     * @param page_id Determines which flash info page to erase
     * @param data_ptr A pointer to the calibration data
     * @param write_length The number of bytes to write
     * @param fref_khz FREF frequency (Khz), must be programmed to valid
     *                 value to enable programming or erasing flash.
     *                 Supported values listed in `FrefFreq`.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*write_info_page)(enum PageIds page_id,
                                         void const*  data_ptr,
                                         size_t       write_length,
                                         uint32_t     fref_khz);

    /**
     * Allows writing the calibration data of the EX10 device.
     *
     * A CRC-16-CCITT will be calculated across the data and appended to the
     * written data.
     *
     * @param data_ptr A pointer to the calibration data
     * @param write_length The number of bytes to write
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*write_calibration_page)(uint8_t const* data_ptr,
                                                size_t         write_length);

    /**
     * Erase an info flash page of the device
     *
     * @param page_id Determines which flash info page to erase
     * @param fref_khz FREF frequency (Khz), must be programmed to valid
     *                 value to enable programming or erasing flash.
     *                 Supported values listed in `FrefFreq`.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*erase_info_page)(enum PageIds page_id,
                                         uint32_t     fref_khz);

    /**
     * Erase the calibration page of the device.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*erase_calibration_page)(void);

    /**
     * Allows writing the stored settings of the EX10 device.
     *
     * A CRC-16-CCITT will be calculated across the data and appended to the
     * written data.
     *
     * @param data_ptr A pointer to the data to write to the settings
     * @param write_length The number of bytes to write
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*write_stored_settings_page)(uint8_t const* data_ptr,
                                                    size_t write_length);

    /**
     * Upload an application image.
     *
     * @param destination  Where in memory to upload the image.
     * @param upload_image Info about the image to upload.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*upload_image)(uint8_t                    destination,
                                      const struct ConstByteSpan upload_image);

    /**
     * Begin the image upload.
     *
     * @param destination  Where in memory to upload the image.
     * @param image_length The total length of the image, in bytes.
     * @param image_chunk  The initial (i.e. first) image chunk to upload.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*upload_start)(uint8_t                    destination,
                                      size_t                     image_length,
                                      const struct ConstByteSpan image_chunk);

    /**
     * Upload part of an image.
     *
     * Call multiple times as need to upload a full image.
     * Each chunk size must not exceed 1021 bytes.
     *
     * @param image_chunk  Info about the image chunk to upload.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*upload_continue)(
        const struct ConstByteSpan image_chunk);

    /**
     * End the image upload and flash the image.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*upload_complete)(void);

    /**
     * Execute the revalidate command and ensure proper validation of
     * the application image in flash.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *
     * @note This function requires that the Impinj Read Chip be executing
     *       in the Bootloader context prior to being run.
     */
    struct ImageValidityFields (*revalidate_image)(void);

    /**
     * Initiate a transfer test. This function executes the Ex10 TransferTest
     * command. This is useful for testing the host to Ex10 wireline
     * communication.
     *
     * @see test_transfer() in commands.h for parameter descriptions.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*test_transfer)(struct ConstByteSpan const* send,
                                       struct ByteSpan*            recv,
                                       bool                        verify);

    /**
     * Wait for the bytes in the Ex10 device EventFifo to be 0 as reported by
     * the EventFifoNumBytes register. The device is queried and the SDK spins
     * in a loop to check when either this condition in met, or the SDK has no
     * more space to read in the FIFO.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     *         If the function returned early due to a full SDK FIFO,
     *         the appropriate error status will be set.
     */
    struct Ex10Result (*wait_for_event_fifo_empty)(void);

    /**
     * Read the device information register from the Impinj Reader Chip.
     * This will return information about the silicon revision.
     *
     * @param [out] struct DeviceInfoFields
     *              The device information register fields.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*get_device_info)(struct DeviceInfoFields* dev_info);

    /**
     * Read the version information from the Application image loaded into
     * the Impinj Reader Chip.
     *
     * @note Unlike the call to get_bootloader_version(), this function does
     *       not reset the Impinj Reader Chip regardless of execution context;
     *       i.e. Calling from Application or Bootloader will not result in
     *       device reset.
     *
     * @param [out] firmware_version
     *              The Application firmware version information.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*get_application_version)(
        struct Ex10FirmwareVersion* firmware_version);

    /**
     * Read the version information for the Impinj Reader Chip Bootloader image.
     *
     * @note Calling this function will result in a reset into the Bootloader,
     *       reading of the Bootloader version information registers, and
     *       then, if the original call was made from the Application, a reset
     *       back to the Application.
     *
     * @param [out] firmware_version
     *              The Bootloader firmware version information.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     * @retval Ex10Result.sdk = Ex10SdkErrorRunLocation if resetting into either
     *         the Bootloader or Application fails.
     */
    struct Ex10Result (*get_bootloader_version)(
        struct Ex10FirmwareVersion* firmware_version);

    /**
     * Read the image validity markers from the Application image loaded into
     * the Impinj Reader Chip.
     *
     * @return struct ImageValidityFields The image  validity register fields.
     */
    struct ImageValidityFields (*get_image_validity)(void);

    /**
     * Read the register from the Impinj Reader Chip Bootloader indicating why
     * the device did not vector into the loaded Application image.
     *
     * @param [out] struct RemainReasonFields
     *         The "remain in bootloader reason" register fields.
     *
     * @return struct Ex10Result
     *         Indicates whether the function call passed or failed.
     */
    struct Ex10Result (*get_remain_reason)(
        struct RemainReasonFields* remain_reason);

    /**
     * Read the SKU of the Impinj Reader Chip.
     *
     * @return enum ProductSku The Ex10 device product SKU.
     */
    enum ProductSku (*get_sku)(void);
};

struct Ex10Protocol const* get_ex10_protocol(void);

#ifdef __cplusplus
}
#endif
