from RadialGauge.Gaugeable import Gaugeable
from GaugeConfigurations import (COLOUR_PRIMARY, COLOUR_TICKS, GAUGE_LABEL, 
TICK_LABEL)
from kivy.uix.label import Label
from kivy.uix.widget import Widget

class GaugeLabel(Gaugeable, Widget):
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
        super(GaugeLabel, self).__init__(angle_start, angle_end, value_max, 
                                         radius, **kwargs)
        
        self.size_hint = (None, None)
        self.pos_hint = { "center_x": 0.5, "center_y": 0.5 }
        self.size = size
        
        cx, cy = self.center
        
        self.reading = Label(font_size = GAUGE_LABEL, 
                           bold = True, color = COLOUR_PRIMARY, 
                           halign = 'center', valign = 'middle',
                           size_hint = (None, None), 
                           size = (radius / 2, radius / 2),
                           center = (cx, cy))
        self.add_widget(self.reading)
        
        self.type = Label(text = type, font_size = TICK_LABEL, 
                           color = COLOUR_TICKS, halign = 'center', 
                           valign = 'middle', size_hint = (None, None), 
                           size = (radius / 2, radius / 2),
                           center = (cx, cy - 0.8 * radius / 2))
        self.add_widget(self.type)
        
    def draw(self, value_curr: float, *args) -> None:
        self.reading.text = str(int(value_curr))