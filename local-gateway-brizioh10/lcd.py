import time

import smbus


class lcd:
    i2c_bus = -1
    lcd_address = 0x3E
    backlight = None

    # I2C expansion address can be:
    # PCF8574T  0x27
    # PCF8574AT 0x3F
    exp_address = 0x27

    def __init__(self, bus_id=0, exp_address=-1):
        self.exp_address = exp_address
        self.i2c_bus = smbus.SMBus(bus_id)
        self.sendcommand(0x38)
        self.sendcommand(0x39)
        self.sendcommand(0x14)  # Internal OSC freq
        self.sendcommand(0x72)  # Set contrast
        self.sendcommand(0x54)  # Power/ICON control/Contrast set
        self.sendcommand(0x6F)  # Follower control
        self.sendcommand(0x0C)  # Display ON
        self.clear()
        # self.backlight=GPIO('PA28','OUTPUT')
        return

    def backlight_on(self):
        self.backlight.on()
        return

    def backlight_off(self):
        self.backlight.off()
        return

    def sendcommand(self, value):
        self.i2c_bus.write_byte_data(self.lcd_address, 0x00, value)
        return

    def senddata(self, value):
        self.i2c_bus.write_byte_data(self.lcd_address, 0x40, value)
        return

    def clear(self):
        self.sendcommand(0x01)
        time.sleep(0.001)
        self.setsinglefont()
        return

    def home(self):
        self.sendcommand(0x03)
        time.sleep(0.001)
        return

    def setcontrast(self, value):
        """
        Set the display contrast
        value = 0 to 15
        """
        self.sendcommand(0x70 + value)
        return

    def setdoublefont(self):
        self.sendcommand(0x30 + 0x0C + 0x01)
        return

    def setsinglefont(self):
        self.sendcommand(0x30 + 0x08 + 0x01)
        return

    def setcurpos(self, x, y):
        if y < 0 or y > 1:
            return
        if x < 0 or x > 15:
            return

        if y == 0:
            self.sendcommand(0x80 + 0x00 + x)
        else:
            self.sendcommand(0x80 + 0x40 + x)
        return

    def putchar(self, value):
        self.senddata(value)
        return

    def putstring(self, string):
        if len(string) == 0:
            return
        if len(string) > 16:
            string = string[0:16]

        for char in string:
            self.putchar(ord(char))
        return

    def backlighton(self):
        self.backled.on()
        return

    def backlightoff(self):
        self.backled.off()
        return
