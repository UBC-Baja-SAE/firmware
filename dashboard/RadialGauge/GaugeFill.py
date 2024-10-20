from RadialGauge.Gaugeable import Gaugeable
from GaugeConfigurations import WIDTH_SMALL, COLOUR_PRIMARY, RATIO_FILL
from kivy.uix.widget import Widget
from kivy.graphics import Color, Line
from kivy.metrics import dp

class GaugeFill(Gaugeable, Widget):
    def __init__(
        self, 
        angle_start: int, 
        angle_end: int, 
        value_max: int,
        radius: int,
        size: tuple,
        **kwargs
    ):
        super(GaugeFill, self).__init__(angle_start, angle_end, value_max, 
                                         radius, **kwargs)
        
        self.size_hint = (None, None)
        self.pos_hint = { "center_x": 0.5, "center_y": 0.5 }
        self.size = size
        
        self.inner_radius = int((1 - RATIO_FILL) * self.radius)
        self.outer_radius = self.radius - WIDTH_SMALL
        
    def draw(self, value_curr: float, *args) -> None:
        self.canvas.clear()
        
        with self.canvas:
            self.drawFill(value_curr)
        
    def drawFill(self, value_curr: float):
        color = COLOUR_PRIMARY
        lines = int(self.radius - self.inner_radius)
        
        for i in range(lines):
            alpha = i / lines * 0.75
            
            Color(color[0], color[1], color[2], alpha)
            Line(
                circle = (self.center_x, self.center_y, 
                          dp(self.inner_radius + i), 90 - self.angle_start,
                          90 - self.getAngle(value_curr)),
                width = dp(WIDTH_SMALL)
            )