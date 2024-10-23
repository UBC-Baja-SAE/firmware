from radial.interface import Gaugeable
from gauge_constants import (
    COLOUR_PRIMARY, COLOUR_TICKS, GAUGE_LABEL, TICK_LABEL
)
from kivy.uix.label import Label
from kivy.uix.widget import Widget

class GaugeReading(Gaugeable, Widget):
    def __init__(
        self, 
        angle_start: int, 
        angle_end: int, 
        value_max: int,
        radius: int,
        type: str,
        size: tuple,
        **kwargs
    ):
        """This class is responsible for displaying the numerical reading of the
        gauge, as well as the unit of measurement.

        Args:
            angle_start (int):  The angle that corresponds to the minimum value
                                of the gauge, measured in degrees 
                                counter-clockwise from the right side of the
                                horizontal axis.
            angle_end (int):    The angle that corresponds to the maximum value,
                                measured in the same way.
            value_max (int):    The maximum value of the gauge reading.
            radius (int):       The radius of the gauge, in pixels.
            type (str):         The label that displays the units for the gauge.
            size (tuple):       The size of the widget, in (width, height).
        """
        super(GaugeReading, self).__init__(angle_start, angle_end, value_max, 
                                         radius, **kwargs)
      
        '''
        Format this widget to use objective sizing, be centered in its parent,
        and have explicit sizing. Note that the size is explicitly passed as a
        parameter, and not in **kwargs.
        '''  
        self.size_hint = (None, None)
        self.pos_hint = { "center_x": 0.5, "center_y": 0.5 }
        self.size = size
        
        cx, cy = self.center
        
        self.reading = Label(
            font_size = GAUGE_LABEL, 
            bold = True,
            color = COLOUR_PRIMARY, 
            halign = 'center',
            valign = 'middle',
            size_hint = (None, None), 
            size = (radius / 2, radius / 2),
            center = (cx, cy)
        )
        self.add_widget(self.reading)
        
        self.type = Label(
            text = type,
            font_size = TICK_LABEL, 
            color = COLOUR_TICKS,
            halign = 'center', 
            valign = 'middle',
            size_hint = (None, None), 
            size = (radius / 2, radius / 2),
            center = (cx, cy - 0.8 * radius / 2)
        )
        self.add_widget(self.type)
        
    def draw(self, value_curr: float, *args) -> None:
        self.reading.text = str(int(value_curr))