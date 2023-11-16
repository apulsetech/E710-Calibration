import clr
clr.AddReference('mcl_pm_NET45')
from mcl_pm_NET45 import usb_pm

class PowerMeter():
    def __init__(self):
        self.pwr = usb_pm()
        
    # todo
    def read_power(self):
        return float(self.pwr.ReadPower())

    def set_rate(self):
        return None
    
    # todo
    def set_frequency(self, freq_mhz):
        self.Freq = freq_mhz

    def get_frequency(self):
        return None
    
    def set_offset_value(self):
        return None
    
    def get_offline_value(self):
        return None
    
    def close(self):
        self.pwr.Close_Sensor()
