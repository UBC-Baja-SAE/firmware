from gauge_constants import (WIDTH_SMALL, COLOUR_PRIMARY, WIDTH_LARGE)
from kivy.uix.widget import Widget
from kivy.graphics import Color, Line
from kivy.metrics import dp

class LinearGaugeFill(Widget):
    def __init__(
        self,
        value_max: int,
        length: int,
        bar_width: int,
        size: tuple,
        **kwargs
    ):
        """This class is responsible for continuously redrawing the fill of the
        gauge.

        Args:
            value_max (int):    The value that corresponds to the maximum
                                reading for the gauge.
            length (int):       The length (or height) of the gauge.
            bar_width (int):    The width of the bar that fills the gauge.
            size (tuple):       The size of the widget, in (width, height)
        """
        super(LinearGaugeFill, self).__init__(**kwargs)
        
        '''
        Format this widget to use objective sizing, be centered in its parent,
        and have explicit sizing. Note that the size is explicitly passed as a
        parameter, and not in **kwargs.
        '''
        self.size_hint = (None, None)
        self.pos_hint = { "center_x": 0.5, "center_y": 0.5 }
        self.size = size
    
        self.length = length
        self.bar_width = bar_width
        self.value_max = value_max
        
    def draw(self, value) -> None:
        self.canvas.clear()
        
        with self.canvas:
            self.drawFill(value)
        
    def drawFill(self, value) -> None:
        color = COLOUR_PRIMARY
        
        value_percentage = value / self.value_max
        
        x_f = WIDTH_LARGE * 2
        y_1 = x_f
        y_2 = (self.length - 2 * x_f) * value_percentage
        lines = self.bar_width
        
        for i in range(lines):
            alpha = i / lines * 0.75
            
            x_i = x_f + lines - i
            
            Color(color[0], color[1], color[2], alpha)
            Line(points = [x_i, y_1, x_i, y_2], width = dp(WIDTH_SMALL))