from kivy.uix.relativelayout import RelativeLayout
from radial.fill import GaugeFill
from radial.ticks import GaugeTicks
from radial.reading import GaugeReading

class RadialGauge(RelativeLayout):
    def __init__(
        self,
        angle_start: int, 
        angle_end: int, 
        value_max: int,
        radius: int,
        ticks: int,
        type: str,
        labelShown: bool,
        **kwargs
    ):
        """This class represents an instance of a radial gauge.

        Args:
            angle_start (int):  The angle that corresponds to the minimum value
                                of the gauge, measured in degrees 
                                counter-clockwise from the right side of the
                                horizontal axis.
            angle_end (int):    The angle that corresponds to the maximum value,
                                measured in the same way.
            value_max (int):    The maximum value of the gauge reading.
            radius (int):       The radius of the gauge, in pixels.
            ticks (int):        The number of major ticks for the gauge.
            type (str):         The label that displays the units for the gauge.
            labelShown (bool):  True if the numerical value of the current gauge
                                reading is shown.
        """
        
        '''
        Format this widget to use objective sizing, and to use sizing and
        positioning passed through **kwargs as key-value pairs.
        '''
        self.size_hint = (None, None)
        self.size = kwargs.pop("size")
        self.center = kwargs.get("center")
        
        super(RadialGauge, self).__init__(**kwargs)
                
        values = (angle_start, angle_end, value_max, radius)
        
        self.fill = GaugeFill(*values, self.size)
        self.ticks = GaugeTicks(*values, ticks, self.size)
        self.reading = GaugeReading(*values, type, self.size)
        self.labelShown = labelShown
        
        for component in [self.ticks, self.fill, self.reading]:
            self.add_widget(component)

    def update(self, value_new):
        self.fill.draw(value_new)
        if self.labelShown is True:
            self.reading.draw(value_new)
    