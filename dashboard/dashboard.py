from kivy.app import App
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.widget import Widget
from kivy.clock import Clock
from kivy.core.window import Window
from kivy.lang import Builder
from radial.gauge import RadialGauge
from linear.gauge import LinearGauge
from math import cos, sin
from gauge_constants import (
    FUEL_CENTER_POSITION,
    FUEL_MAX_VALUE,
    FUEL_SYMBOL_MAX,
    FUEL_SYMBOL_MIN,
    FUEL_TICKS,
    LINEAR_BAR_WIDTH,
    LINEAR_LENGTH,
    LINEAR_SIZE,
    RADIAL_END_ANGLE,
    RADIAL_SIZE,
    RADIAL_START_ANGLE,
    RADIUS,
    SCREEN_SIZE,
    SPEEDOMETER_CENTER_POSITION,
    SPEEDOMETER_LABEL,
    SPEEDOMETER_MAX_VALUE,
    SPEEDOMETER_SHOW_LABEL,
    SPEEDOMETER_TICKS,
    TACHOMETER_CENTER_POSITION,
    TACHOMETER_LABEL,
    TACHOMETER_MAX_VALUE,
    TACHOMETER_SHOW_LABEL,
    TACHOMETER_TICKS,
    TEMPERATURE_CENTER_POSITION,
    TEMPERATURE_MAX_VALUE,
    TEMPERATURE_SYMBOL_MAX,
    TEMPERATURE_SYMBOL_MIN,
    TEMPERATURE_TICKS
)

Window.size = SCREEN_SIZE
Window.fullscreen = True

# Pre-load the layout files for the images. Use the corresponding created
# classes to input the files.
Builder.load_file("./assets/layout/logo.kv")
Builder.load_file("./assets/layout/fuel.kv")
Builder.load_file("./assets/layout/temp.kv")
# Loaded from .kv files.
class Logo(Widget): pass
class Fuel(Widget): pass
class Temp(Widget): pass
    
class DashboardApp(App):
    def __init__(self, **kwargs):
        """Represents an instance of the dashboard GUI application"""
        super(DashboardApp, self).__init__(**kwargs)
        
        '''
        Used to simulate the changing values, CHANGE LATER
        '''
        self.time = 0

    def build(self):
        self.layout = FloatLayout(size = SCREEN_SIZE)
        
        self.layout.add_widget(Logo())
        
        self.speedometer = RadialGauge(
            RADIAL_START_ANGLE,
            RADIAL_END_ANGLE,
            SPEEDOMETER_MAX_VALUE,
            RADIUS,
            SPEEDOMETER_TICKS,
            SPEEDOMETER_LABEL,
            SPEEDOMETER_SHOW_LABEL,
            size = RADIAL_SIZE,
            center = SPEEDOMETER_CENTER_POSITION
        )
        self.layout.add_widget(self.speedometer)
        
        self.tachometer = RadialGauge(
            RADIAL_START_ANGLE,
            RADIAL_END_ANGLE,
            TACHOMETER_MAX_VALUE,
            RADIUS,
            TACHOMETER_TICKS,
            TACHOMETER_LABEL,
            TACHOMETER_SHOW_LABEL,
            size = RADIAL_SIZE,
            center = TACHOMETER_CENTER_POSITION
        )
        self.layout.add_widget(self.tachometer)
        
        self.fuel = LinearGauge(
            FUEL_MAX_VALUE,
            LINEAR_LENGTH,
            LINEAR_BAR_WIDTH,
            FUEL_TICKS,
            FUEL_SYMBOL_MIN,
            FUEL_SYMBOL_MAX,
            size = LINEAR_SIZE,
            center = FUEL_CENTER_POSITION
        )
        self.fuel.add_widget(Fuel())
        self.layout.add_widget(self.fuel)
        
        self.temp = LinearGauge(
            TEMPERATURE_MAX_VALUE,
            LINEAR_LENGTH,
            LINEAR_BAR_WIDTH,
            TEMPERATURE_TICKS,
            TEMPERATURE_SYMBOL_MIN,
            TEMPERATURE_SYMBOL_MAX,
            size = LINEAR_SIZE,
            center = TEMPERATURE_CENTER_POSITION
        )
        self.temp.add_widget(Temp())
        self.layout.add_widget(self.temp)
        
        '''Starts simulation'''
        Clock.schedule_interval(self.simulateChange, 1 / 20)
        
        return self.layout
        
    '''
    Simulates fluctuating value readings for sensors.
    '''
    def simulateChange(self, dt):
        speed = 70 / 2 * (1 + sin(self.time * 3))
        self.speedometer.update(speed)
        
        tach = 5 / 2 * (1 + cos(self.time * 2))
        self.tachometer.update(tach)
        
        temp = 150 / 2 * (1 + sin(self.time / 2))
        self.temp.update(temp)
        
        fuel = 150 / 2 * (1 + cos(self.time / 1.5))
        self.fuel.update(fuel)

        self.time += 1 / 20
    
if __name__ == '__main__':
    DashboardApp().run()


