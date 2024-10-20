from kivy.uix.relativelayout import RelativeLayout
from .LinearGaugeTicks import LinearGaugeTicks
from .LinearGaugeFill import LinearGaugeFill

class LinearGauge(RelativeLayout):
    def __init__(
        self, 
        value_max: int, 
        length: int, 
        width: int, 
        bar_width: int, 
        ticks: int,
        sym_min: str,
        sym_max: str,
        **kwargs
    ):
        self.size_hint = (None, None)
        self.size = kwargs.pop("size")
        self.center = kwargs.get("center")
        
        super(LinearGauge, self).__init__(**kwargs)
    
        self.fill = LinearGaugeFill(value_max, length, bar_width, self.size, **kwargs)
        self.ticks = LinearGaugeTicks(length, bar_width, ticks, sym_min, 
                                      sym_max, self.size, **kwargs)
        
        for component in [self.ticks, self.fill]:
            self.add_widget(component)
        
    def update(self, value, *args):
        self.fill.draw(value)