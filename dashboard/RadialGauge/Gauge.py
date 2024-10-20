from kivy.uix.relativelayout import RelativeLayout
from RadialGauge.GaugeFill import GaugeFill
from RadialGauge.GaugeTicks import GaugeTicks
from RadialGauge.GaugeLabel import GaugeLabel

class Gauge(RelativeLayout):
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
        self.size_hint = (None, None)
        self.size = kwargs.get("size")
        self.center = kwargs.get("center")
        
        super(Gauge, self).__init__(**kwargs)
                
        gaugeable = (angle_start, angle_end, value_max, radius)
        
        self.fill = GaugeFill(*gaugeable, self.size)
        self.ticks = GaugeTicks(*gaugeable, ticks, self.size)
        self.reading = GaugeLabel(*gaugeable, type, self.size)
        self.labelShown = labelShown
        
        for component in [self.ticks, self.fill, self.reading]:
            self.add_widget(component)

    def update(self, value_new):
        self.fill.draw(value_new)
        if self.labelShown is True:
            self.reading.draw(value_new)
    