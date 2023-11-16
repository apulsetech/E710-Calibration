#############################################################################
#                  IMPINJ CONFIDENTIAL AND PROPRIETARY                      #
#                                                                           #
# This source code is the property of Impinj, Inc. Your use of this source  #
# code in whole or in part is subject to your applicable license terms      #
# from Impinj.                                                              #
# Contact support@impinj.com for a copy of the applicable Impinj license    #
# terms.                                                                    #
#                                                                           #
# (c) Copyright 2020 - 2022 Impinj, Inc. All rights reserved.               #
#                                                                           #
#############################################################################
import time
import numpy as np


class StopWatch:
    """ Measures elapsed and delta time """

    _times = []
    _categories = []
    _labels = []

    def start(self):
        self.split('START')

    def stop(self):
        self.split('STOP')

    def split(self, category='', label=''):
        self._times.append(time.perf_counter())
        self._categories.append(category)
        self._labels.append(label)

    @property
    def times(self):
        return np.array(self._times) - self._times[0]

    @property
    def labels(self):
        return np.array(self._labels)

    @property
    def categories(self):
        return np.array(self._categories)

    @property
    def lap_times(self):
        return self.times[1:] - self.times[:-1]

    @property
    def lap_labels(self):
        return self.labels[1:]

    @property
    def lap_categories(self):
        return self.categories[1:]
