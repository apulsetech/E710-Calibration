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
RFR Shim contains state variable as well as knowledge of how to utilize
the EX10 API specific to the R807 board. Contained within are the proper
calls for RFR to call and helpers to work with EX10 devices.
This will eventually customer facing, but until it has been vetted, keep
in the utils folder
"""

from __future__ import (division, absolute_import, print_function,
                        unicode_literals)
import collections
import math
import time

from enum import Enum, IntEnum
from py2c_interface.py2c_python_wrapper import *
from typing import NamedTuple


class Waveform(Enum):
    # pylint: disable=locally-disabled, invalid-name
    # pylint: disable=locally-disabled, too-few-public-methods
    """
    Waveform type to be run on start_transmit
    """
    CW = "Continuous_Wave"
    PRBS = "Random_Modulated_Data"
    ETSI = "Etsi_Burst"
    Singulate = "Singulate_Tags"

class SuppliedFreqType(Enum):
    # pylint: disable=locally-disabled, invalid-name
    # pylint: disable=locally-disabled, too-few-public-methods
    """
    Form of frequency passed
    """
    SDKTABLE = "sdk_hop_table"
    CUSTOMTABLE = "custom_hop_table_list"
    NOREGULATORY = "single_frequency_no_reg"

class InventorySearchMode(IntEnum):
    """
    All possible inventory modes if the Waveform type is
    Singulate_Tags.
    """
    Reader_Selected = 0
    Single_Target = 1
    Dual_Target = 2
    Single_Target_With_Suppression = 3
    No_Target = 4
    Single_Target_BtoA = 5
    Dual_Target_with_BtoA_Select = 6


# One can use this named tuple rather than using the
# individual gets and sets.
class YukonTxConfigBase(collections.namedtuple("YukonTxConfigBase", ['region',
                                                             'transmit_power_cdbm',
                                                             'frequency_mhz',
                                                             'rf_mode',
                                                             'inventory_search_mode',
                                                             'waveform',
                                                             'antenna_port',
                                                             'intelligent_antenna_mode',
                                                             'gen2_tag_population',
                                                             'gen2_session'])):
    def __new__(cls,
        region,
        transmit_power_cdbm,
        frequency_mhz,
        rf_mode,
        inventory_search_mode,
        waveform,
        antenna_port,
        intelligent_antenna_mode,
        gen2_tag_population,
        gen2_session):
        return super(YukonTxConfigBase, cls).__new__(cls,
            region,
            transmit_power_cdbm,
            frequency_mhz,
            rf_mode,
            inventory_search_mode,
            waveform,
            antenna_port,
            intelligent_antenna_mode,
            gen2_tag_population,
            gen2_session)


class TxReaderApis(NamedTuple):
    """
    Wrapper for encapsulating dependency injection

    To avoid calls to...
    self.py2c.ex10_typical_board_setup
    in order to avoid gpio collisions in certain applications.
    """
    ex10_py2c_intercept: GetEx10InterfacesIntercept
    hw_dev: Ex10Py2CWrapper

class FreqListIterator(object):
    def __init__(self, freq_list):
        self.list_size = len(freq_list)
        assert self.list_size is not 0
        self.freq_list = freq_list
        self.cur_iter = 0

    def get_next_freq(self):
        next_freq = self.freq_list[self.cur_iter]
        self.cur_iter += 1
        if self.cur_iter >= self.list_size:
            self.cur_iter = 0
        return next_freq

class Ex10TxTestShimBase(object):
    # pylint: disable=locally-disabled, too-many-public-methods
    # pylint: disable=locally-disabled, too-many-instance-attributes
    # pylint: disable=locally-disabled, protected-access
    """
    Controls of the EX10 API for RFR
    """
    def __init__(self, tx_reader_apis: TxReaderApis):
        """
        Create an Op-oriented interface to an Ex10 chip.

        Note: gpio control is 'owned' by whomever is instantiating this instance.
        This avoids gpio control issues.
        """
        # Local state variables for Tx testing
        self.py2c = tx_reader_apis.hw_dev
        self._inventory_duration = 30
        self._tx_waveform = None

        self.inventory_search_mode = InventorySearchMode.Dual_Target
        self.antenna_port = 1
        self.region = 'FCC'
        self.rf_mode = 13
        self.session = 1
        self.population = 1
        self.transmit_power_cdbm = 0
        self.freq_mhz = 916.25
        self.finished_tasks = True
        self.finish_soon = False
        if not tx_reader_apis:
            raise Exception('No tx_reader_apis were specified.')
        elif tx_reader_apis.hw_dev == None:
            # needed to ensure single CSDK being used between threads
            raise Exception('hw_dev must be specified in tx_reader_apis.')
        else:
            self.ex10_reader = tx_reader_apis.ex10_py2c_intercept.reader
            self.ex10_ops = tx_reader_apis.ex10_py2c_intercept.ops
            self.ex10_protocol = tx_reader_apis.ex10_py2c_intercept.protocol
            self.helper = tx_reader_apis.ex10_py2c_intercept.helpers
            self.version = tx_reader_apis.ex10_py2c_intercept.version
            self.rf_power = tx_reader_apis.ex10_py2c_intercept.rf_power

    def _internal_set_region(self, region_name):
        """
        Update the region of operation. This allows for switching the region on
        the fly for test purposes.
        :param region_name: Must be one of the ShortName entries found in the
                            regions.yml file.
        """
        region_id = self.py2c.get_ex10_default_region_names().get_region_id(region_name.encode('ascii'))
        # Set to Null to clear overrides and custom settings
        self.py2c.get_ex10_regulatory().set_region(region_id, None)
        # Now set the active region again to pll from the base region
        self.py2c.get_ex10_active_region().set_region(region_id, TCXO_FREQ_KHZ)

    def cleanup(self):
        """ Cleanup the shim layer """
        self.py2c.ex10_typical_board_teardown()
        time.sleep(.25)

    def get_reader(self):
        """ Returns the reader instance for calls by the tester """
        return self.ex10_reader

    def set_tx_configs(self, tx_configs, thread_lock):
        """
        Takes a named tuple of YukonTxConfigBase to set the configs for Tx
        testing, then kicks of the test.
        :param tx_configs: configs around the params used for testing
        :param thread_lock: thread lock is a semaphore lock. It is
                            controlled from outside this class and based
                            on the lock state will keep the test looping
                            or cause it to clean up and return.
        """
        if thread_lock is None:
            raise ValueError("Thread needed for proper test termination")

        self.region = tx_configs.region
        # Ensure initialization of the pointers in the shim

        self.finished_tasks = False

        self.transmit_power_cdbm = tx_configs.transmit_power_cdbm
        self.freq_mhz = tx_configs.frequency_mhz
        self.rf_mode = tx_configs.rf_mode
        self.inventory_search_mode = tx_configs.inventory_search_mode
        self.waveform = tx_configs.waveform
        self.antenna_port = tx_configs.antenna_port
        self.tag_population = tx_configs.gen2_tag_population
        self.session = tx_configs.gen2_session

        # This will loop on starting an maintaining Tx Waveforms until
        # the semaphore lock tells us to stop.
        self._start_transmit(thread_lock)
        self.finished_tasks = True
        self.cleanup()

    def get_tx_configs(self):
        """
        Returns a type of YukonTxConfigBase with the current configs
        of the test setup
        """
        current_configs = YukonTxConfigBase(
            region=self.region,
            transmit_power_cdbm=self.transmit_power_cdbm,
            frequency_mhz=self.freq_mhz,
            rf_mode=self.rf_mode,
            inventory_search_mode=self.inventory_search_mode,
            waveform=self.waveform,
            antenna_port=self.antenna_port,
            intelligent_antenna_mode=False,
            gen2_tag_population=self.tag_population,
            gen2_session=self.session,
        )
        return current_configs

    def _is_ex10_error(self, ex10_result):
        if ex10_result.error is True:
            print('Ex10 Error encountered')
            self.py2c.print_ex10_result(ex10_result)
            # In an error occurred, print and flush the Ex10 Event FIFO
            self.py2c.ex10_discard_packets(True, True, True)
            return True

        return False

    def _empty_reports(self):
        """ Get inventory reports and throw them away """
        self.helper.discard_packets(False, True, False)

    def _start_inventory(self, freq_type, thread_lock):
        """
        Configure and run inventory rounds
        """
        # Put together configurations for the inventory round
        inventory_config = InventoryRoundControlFields()
        inventory_config.initial_q = 15
        inventory_config.max_q = 15
        inventory_config.min_q = 0
        inventory_config.num_min_q_cycles = 1
        inventory_config.fixed_q_mode = True
        inventory_config.q_increase_use_query = False
        inventory_config.q_decrease_use_query = False
        inventory_config.session = self.session
        inventory_config.select = 1
        inventory_config.target = self.inventory_search_mode.value
        inventory_config.halt_on_all_tags = False
        inventory_config.fast_id_enable = False
        inventory_config.tag_focus_enable = False

        inventory_config_2 = InventoryRoundControl_2Fields()
        inventory_config_2.max_queries_since_valid_epc = 16

        stop_conditions = StopConditions()
        stop_conditions.max_number_of_tags   = 0
        stop_conditions.max_duration_ms      = 0
        stop_conditions.max_number_of_rounds = 0
        print('starting inventory')

        if freq_type == SuppliedFreqType.NOREGULATORY:
            # Runs inventory until we stop it, aka when user signals
            while True:
                self.ex10_ops.wait_op_completion()
                self.py2c.get_ex10_active_region().set_single_frequency(int(self.freq_khz))
                ex10_result = self.ex10_reader.inventory(self.antenna_port,
                                                         self.rf_mode,
                                                         self.transmit_power_cdbm,
                                                         inventory_config,
                                                         inventory_config_2,
                                                         False,
                                                         True)
                if self._is_ex10_error(ex10_result):
                    # Ignore specific errors which come from shutting down the
                    # thread during run time.
                    if (ex10_result.device_status.ops_status.error == \
                        OpsStatus.ErrorAggregateInnerOpError) or \
                        ex10_result.device_status.ops_status.busy:
                        pass
                    else:
                        raise Exception('An error occurred during inventory')

                self._empty_reports()
                # If the thread lock becomes unlocked, we stop and return
                if thread_lock.locked() or self.finish_soon == True:
                    return
        elif freq_type == SuppliedFreqType.SDKTABLE:
            self.ex10_ops.wait_op_completion()
            ex10_result = self.ex10_reader.continuous_inventory(self.antenna_port,
                                                                self.rf_mode,
                                                                self.transmit_power_cdbm,
                                                                inventory_config,
                                                                inventory_config_2,
                                                                False,
                                                                stop_conditions,
                                                                False, False)
            # Runs until we stop it, aka when user signals
            while True:
                self._empty_reports()
                if self._is_ex10_error(ex10_result):
                    print('error_occurred')
                    self.py2c.print_ex10_result(ex10_result)
                    ex10_result = self.ex10_reader.continuous_inventory(self.antenna_port,
                                                                        self.rf_mode,
                                                                        self.transmit_power_cdbm,
                                                                        inventory_config,
                                                                        inventory_config_2,
                                                                        False,
                                                                        stop_conditions,
                                                                        False, False)
                # If the thread lock becomes unlocked, we stop and return
                if thread_lock.locked() or self.finish_soon == True:
                    return
        elif freq_type == SuppliedFreqType.CUSTOMTABLE:
            # Runs inventory until we stop it, aka when user signals
            self.freq_iter = FreqListIterator(self.freq_khz)
            while True:
                self.ex10_ops.wait_op_completion()
                self.py2c.get_ex10_active_region().set_single_frequency(
                    int(self.freq_iter.get_next_freq()))
                ex10_result = self.ex10_reader.inventory(self.antenna_port,
                                                         self.rf_mode,
                                                         self.transmit_power_cdbm,
                                                         inventory_config,
                                                         inventory_config_2,
                                                         False,
                                                         False)
                if self._is_ex10_error(ex10_result):
                    # Ignore specific errors which come from shutting down the
                    # thread during run time.
                    if (ex10_result.device_status.ops_status.error == \
                        OpsStatus.ErrorAggregateInnerOpError) or \
                        ex10_result.device_status.ops_status.busy:
                        pass
                    else:
                        raise Exception('An error occurred during inventory')

                self._empty_reports()
                # If the thread lock becomes unlocked, we stop and return
                if thread_lock.locked() or self.finish_soon == True:
                    return

    def _last_op_ramp_down(self):
        """ Returns true if the previous op run was ramp down """
        ops_status_reg = self.ex10_ops.read_ops_status()
        return ops_status_reg.op_id

    def _call_cw(self, freq, remain_on):
        self.ex10_ops.wait_op_completion()
        ex10_result = self.ex10_reader.cw_test(self.antenna_port,
                                               self.rf_mode,
                                               self.transmit_power_cdbm,
                                               freq,
                                               remain_on)
        if self._is_ex10_error(ex10_result):
            raise Exception('An error occurred during cw_test')

    def _start_cw(self, freq_type, thread_lock):
        """
        Loop on calling CW whenever CW is ramped down
        """
        while True:
            if not self.rf_power.get_cw_is_on():
                if freq_type == SuppliedFreqType.NOREGULATORY:
                    self._call_cw(self.freq_khz, True)
                elif freq_type == SuppliedFreqType.CUSTOMTABLE:
                    # Setup an iterator for the custom table
                    self.freq_iter = FreqListIterator(self.freq_khz)
                    self._call_cw(self.freq_iter.get_next_freq(), False)
                elif freq_type == SuppliedFreqType.SDKTABLE:
                    # Call CW with regulatory timers and reader controlled freq
                    self._call_cw(0, False)
            self._empty_reports()
            # If the thread lock becomes unlocked, we stop and return
            if thread_lock.locked() or self.finish_soon == True:
                return

    def _call_prbs(self, freq, remain_on):
        ex10_result = self.ex10_reader.prbs_test(
            self.antenna_port,
            self.rf_mode,
            self.transmit_power_cdbm,
            freq,
            remain_on)

        if self._is_ex10_error(ex10_result):
            # regulatory timeout is expected
            if not ex10_result.device_status.ops_status.busy:
                raise Exception('An error occurred during prbs')

    def _start_prbs(self, freq_type, thread_lock):
        """
        Turn on PRBS waveform. PRBS does not finish unless stopped by the
        user or regulatory timers
        """
        if freq_type == SuppliedFreqType.NOREGULATORY:
            self._call_prbs(self.freq_khz, True)
            while True:
                self._empty_reports()
                # If the thread lock becomes unlocked, we stop and return
                if thread_lock.locked() or self.finish_soon == True:
                    return
        elif freq_type == SuppliedFreqType.SDKTABLE:
            while True:
                self._call_prbs(0, False)
                self._empty_reports()
                # If the thread lock becomes unlocked, we stop and return
                if thread_lock.locked() or self.finish_soon == True:
                    return
        elif freq_type == SuppliedFreqType.CUSTOMTABLE:
            self.freq_iter = FreqListIterator(self.freq_khz)
            cur_freq = self.freq_khz[0]
            while True:
                if not self.rf_power.get_cw_is_on():
                    cur_freq = self.freq_iter.get_next_freq()
                self._call_prbs(cur_freq, False)
                self._empty_reports()
                # If the thread lock becomes unlocked, we stop and return
                if thread_lock.locked() or self.finish_soon == True:
                    return

    def _start_etsi_burst(self, freq_type, thread_lock):
        """
        Turn on ETSI Burst waveform. Etsi Burst does not finish unless
        stopped by the user.
        """
        # Ensure certain parameters for Etsi Burst
        if self._session is not 1:
            raise ValueError('Sessions should be 1 for etsi burst')
        # initial q is hardcoded to 15 to match speedway

        inventory_config = InventoryRoundControlFields()
        inventory_config.initial_q = 15
        inventory_config.max_q = 15
        inventory_config.min_q = 0
        inventory_config.num_min_q_cycles = 1
        inventory_config.fixed_q_mode = True
        inventory_config.q_increase_use_query = False
        inventory_config.q_decrease_use_query = False
        inventory_config.session = self._session
        inventory_config.select = 1
        inventory_config.target = 0
        inventory_config.halt_on_all_tags = False
        inventory_config.fast_id_enable = False
        inventory_config.tag_focus_enable = False

        inventory_config_2 = InventoryRoundControl_2Fields()
        inventory_config_2.max_queries_since_valid_epc = 16

        # RFR interface does not specify etsi burst time. For speedway,
        # the on and off time are baked in. We pass them in here. The
        # values should be 40ms on and 5ms off.
        if freq_type != SuppliedFreqType.NOREGULATORY:
            raise ValueError("Etsi Burst must be used with single frequency")

        self.ex10_ops.wait_op_completion()
        ex10_result = self.ex10_reader.inventory(self.antenna_port,
                                                 self.rf_mode,
                                                 self.transmit_power_cdbm,
                                                 inventory_config,
                                                 inventory_config_2,
                                                 None,
                                                 False)
        if self._is_ex10_error(ex10_result):
            raise Exception('An error occurred during inventory')
        self.ex10_reader.stop_transmitting()
        ex10_result = self.ex10_reader.etsi_burst_test(inventory_config,
                                                       inventory_config_2,
                                                       self.antenna_port,
                                                       self.rf_mode,
                                                       self.transmit_power_cdbm,
                                                       40, 5, self.freq_khz)
        if self._is_ex10_error(ex10_result):
            raise Exception('An error occurred during ETSI burst')

        # The etsi burst op will continue running until user stop.
        while True:
            self._empty_reports()
            # If the thread lock passed in becomes unlocked, we know to return
            if thread_lock.locked() or self.finish_soon == True:
                return

    def _set_initial_waveform(self):
        ex10_result = self.ex10_ops.wait_op_completion()
        if self._is_ex10_error(ex10_result):
            raise Exception('An error occurred waiting for a previous op')

        if self.freq_khz is None:
            # leave it to the sdk
            freq_type = SuppliedFreqType.SDKTABLE
        elif isinstance(self.freq_khz, (int, float)):
            if self.freq_khz == 0:
                # leave it to the sdk
                freq_type = SuppliedFreqType.SDKTABLE
            else:
                # single freq supplied, so ramp and stay there
                freq_type = SuppliedFreqType.NOREGULATORY
        elif isinstance(self.freq_khz, list):
            # list of any size. loop through while ramping to each
            freq_type = SuppliedFreqType.CUSTOMTABLE
        else:
            raise ValueError("Expected a single frequency or None")

        if self._tx_waveform == None:
            self._tx_waveform = Waveform.Singulate

        return freq_type

    def _start_transmit(self, thread_lock):
        """
        Decide which waveform to start up
        """
        freq_type = self._set_initial_waveform()

        # Call the appropriate logic based on the waveform
        if self._tx_waveform == Waveform.CW:
            self._start_cw(freq_type, thread_lock)
        elif self._tx_waveform == Waveform.Singulate:
            self._start_inventory(freq_type, thread_lock)
        elif self._tx_waveform == Waveform.PRBS:
            self._start_prbs(freq_type, thread_lock)
        elif self._tx_waveform == Waveform.ETSI:
            self._start_etsi_burst(freq_type, thread_lock)
        else:
            raise ValueError("Specified waveform not supported currently")

        # Ensure that we are no longer transmitting
        self.ex10_reader.stop_transmitting()

    def _get_initial_q(self):
        return math.floor(math.log(self._population, 2))

    @property
    def region(self):
        return self._region

    @region.setter
    def region(self, region):
        assert region is not None
        self._region = region.replace(" ", "_")
        self._internal_set_region(self._region)

    @property
    def transmit_power_cdbm(self):
        return self._transmit_power_cdbm

    @transmit_power_cdbm.setter
    def transmit_power_cdbm(self, power_cdbm):
        self._transmit_power_cdbm = int(power_cdbm)

    @property
    def freq_mhz(self):
        return self._freq_mhz

    @freq_mhz.setter
    def freq_mhz(self, freq_mhz):
        if freq_mhz == None:
            self._freq_mhz = 0
        else:
            self._freq_mhz = freq_mhz
        # Now if we got a mhz list, translate the list to khz
        if isinstance(freq_mhz, list):
            new_khz_list = []
            for list_mhz in freq_mhz:
                new_khz_list.append(int(list_mhz*1000))
            self._freq_khz = new_khz_list
        else:
            self._freq_khz = int(self._freq_mhz*1000)

    @property
    def freq_khz(self) -> int:
        return self._freq_khz

    @freq_khz.setter
    def freq_khz(self, freq_khz) -> int:
        if freq_khz == None:
            self._freq_khz = 0
        else:
            self._freq_khz = freq_khz

        # Now if we got a khz list, translate the list to mhz
        if isinstance(freq_khz, list):
            new_mhz_list = []
            for list_khz in freq_khz:
                new_mhz_list.append(list_khz / 1000)
            self._freq_mhz = new_mhz_list
        else:
            self._freq_mhz = self._freq_khz / 1000

    @property
    def rf_mode(self) -> int:
        return self._rf_mode

    @rf_mode.setter
    def rf_mode(self, rf_mode: int):
        # pylint: disable=locally-disabled, bad-builtin
        self._rf_mode = rf_mode

    @property
    def inventory_search_mode(self):
        # NOTE: Currently supports single and dual target
        return self._inventory_search_mode

    @inventory_search_mode.setter
    def inventory_search_mode(self, inventory_mode):
        if (inventory_mode != InventorySearchMode.Single_Target and
                inventory_mode != InventorySearchMode.Dual_Target):
            raise ValueError("Specified mode unsupported at this time")
        self._inventory_search_mode = inventory_mode

    @property
    def waveform(self):
        return self._tx_waveform

    @waveform.setter
    def waveform(self, waveform):
        self._tx_waveform = waveform

    @property
    def antenna_port(self):
        return self._antenna_port

    @antenna_port.setter
    def antenna_port(self, antenna):
        self._antenna_port = antenna

    @property
    def ro_spec_active(self):
        # NOTE: RO spec is an LLRP setting for transmit on, we don't support transmit off
        # Calling code should cache state and manage ro_spec True / False transitions if necessary
        return True

    @property
    def tag_population(self):
        return self._population

    @tag_population.setter
    def tag_population(self, population):
        self._population = population

    @property
    def session(self):
        return self._session

    @session.setter
    def session(self, session):
        self._session = session

    @property
    def shim_version(self):
        current_version = 3
        return current_version
