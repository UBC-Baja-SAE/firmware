from kivy.uix.widget import Widget
from kivy.graphics import Color, Line
from kivy.metrics import dp
from kivy.uix.label import Label
from kivy.lang import Builder
from GaugeConfigurations import (COLOUR_GRADIENT, COLOUR_TICKS, RATIO_GRADIENT,
                                 WIDTH_SMALL, RATIO_MINOR, LINEAR_GAUGE_LABEL)

class LinearGaugeTicks(Widget):
    def __init__(
        self,
        length: int,
        bar_width: int,
        ticks: int,
        sym_min: str, 
        sym_max: str,
        size: tuple,
        **kwargs
    ):
        super(LinearGaugeTicks, self).__init__(**kwargs)
        
        self.size_hint = (None, None)
        self.pos_hint = { "center_x": 0.5, "center_y": 0.5 }
        self.size = size
        
        self.length = length
        self.bar_width = bar_width
        self.ticks = ticks
        self.sym_min = sym_min
        self.sym_max = sym_max
        
        self.draw()
        
    def draw(self, *args) -> None:
        with self.canvas:
            self.drawGradient()
            self.drawOutline()
            self.drawTicks()
            self.drawLabel(self.sym_min)
            self.drawLabel(self.sym_max)
            
    def drawOutline(self) -> None:
        Color(*COLOUR_TICKS)
        Line(points = [0, 0, 0, self.size[1]], width = dp(WIDTH_SMALL))
        
    def drawGradient(self) -> None:
        color = COLOUR_GRADIENT
        lines = int(self.bar_width * RATIO_GRADIENT)
        
        for i in range(lines):
            alpha = i / lines * 0.25
            
            x_i = lines - i
            
            Color(color[0], color[1], color[2], alpha)
            Line(points = [x_i, 0, x_i, self.length], width = dp(WIDTH_SMALL))
            
    def drawTicks(self) -> None:
        self.drawTick(1, 0)
        
        full_interval = self.length / (self.ticks - 1)
        for i in range(1, self.ticks):
            y_full = i * full_interval
            y_half = y_full - full_interval / 2

            self.drawTick(RATIO_MINOR, y_half)
            self.drawTick(1, y_full)
    
    def drawTick(self, ratio: float, y: int) -> None:
        Color(*COLOUR_TICKS)
        Line(points = [0, y, self.bar_width * ratio, y], 
             width = dp(WIDTH_SMALL))
        
    def drawLabel(self, text: str) -> None:
        size = dp(20)
        
        x, y = self.size
        
        if text is self.sym_min:
            y = 0
            
        label = Label(text = str(text), color = COLOUR_TICKS, halign = 'center',
            font_size = LINEAR_GAUGE_LABEL, valign = 'middle',
            size = (size, size), 
            center = (self.bar_width + size / 2, y))
        
        self.add_widget(label)