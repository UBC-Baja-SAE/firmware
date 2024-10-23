from kivy.uix.relativelayout import RelativeLayout
from .ticks import LinearGaugeTicks
from .fill import LinearGaugeFill

class LinearGauge(RelativeLayout):
    def __init__(
        self, 
        value_max: int, 
        length: int,
        bar_width: int, 
        ticks: int,
        sym_min: str,
        sym_max: str,
        **kwargs
    ):
        """This class represents an instance of a linear gauge.

        Args:
            value_max (int):    The value that corresponds to the maximum
                                reading for the gauge.
            length (int):       The length (or height) of the gauge.
            bar_width (int):    The width of the bar that fills the gauge.
            ticks (int):        The number of [major] tick marks, with one tick mark 
                                in-between each major increment.
            sym_min (str):      The string at the bottom of the gauge
            sym_max (str):      The string at the top of the gauge
        """
        
        '''
        Format this widget to use objective sizing, and to use sizing and
        positioning passed through **kwargs as key-value pairs.
        '''
        self.size_hint = (None, None)
        self.size = kwargs.pop("size")
        self.center = kwargs.get("center")
        
        super(LinearGauge, self).__init__(**kwargs)
    
        self.fill = LinearGaugeFill(
            value_max,
            length,
            bar_width,
            self.size,
            **kwargs
        )
        self.ticks = LinearGaugeTicks(
            length,
            bar_width,
            ticks,
            sym_min, 
            sym_max,
            self.size,
            **kwargs
        )
        
        for component in [self.ticks, self.fill]:
            self.add_widget(component)
        
    def update(self, value: float, *args) -> None:
        """Updates the value of this gauge.

        Args:
            value (float):  The value to update the gauge to.
        """
        self.fill.draw(value)