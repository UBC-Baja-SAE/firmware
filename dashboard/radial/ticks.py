from radial.interface import Gaugeable
from gauge_constants import (
    COLOUR_GRADIENT, COLOUR_TICKS, WIDTH_SMALL, WIDTH_LARGE, TICK_LABEL,
    RADIAL_TICK_RATIO_LARGE, RADIAL_TICK_RATIO_SMALL, RATIO_GRADIENT, 
)
from kivy.uix.widget import Widget
from kivy.uix.label import Label
from kivy.graphics import Color, Line
from kivy.metrics import dp
from math import cos, sin, radians

class GaugeTicks(Gaugeable, Widget):
    def __init__(
        self, 
        angle_start: int, 
        angle_end: int, 
        value_max: int,
        radius: int,
        ticks: int,
        size: tuple,
        **kwargs
    ):
        super(GaugeTicks, self).__init__(
            angle_start, 
            angle_end, 
            value_max, 
            radius, 
            **kwargs
        )
        
        self.size_hint = (None, None)
        self.pos_hint = { "center_x": 0.5, "center_y": 0.5 }
        self.size = size
        
        self.ticks = ticks
        
        self.draw()
        
    def draw(self, *args) -> None:
        with self.canvas:
            self.drawGradient()
            self.drawOutline()
            self.drawTicks()
            
    def drawOutline(self) -> None:
        Color(*COLOUR_TICKS)
        Line(
            circle = (
                self.center_x,
                self.center_y,
                dp(self.radius),
                self.angle_start - 90,
                self.angle_end - 90
            ),
            width = dp(WIDTH_SMALL)
        )
        
    def drawGradient(self) -> None:
        color = COLOUR_GRADIENT
        start_radius = int((1 - RATIO_GRADIENT) * self.radius)
        lines = int(self.radius - start_radius)
        
        for i in range(lines):
            alpha = i * i / lines / lines * 0.25
            
            Color(color[0], color[1], color[2], alpha)
            Line(
                circle = (
                    self.center_x,
                    self.center_y,
                    dp(start_radius + i), 
                    self.angle_start - 90,
                    self.angle_end - 90
                ),
                width = dp(WIDTH_SMALL)
            )
    
    def drawTicks(self) -> None:
        self.drawTick(self.angle_start, RADIAL_TICK_RATIO_LARGE, WIDTH_LARGE, 0)
        
        full_interval = (self.angle_end - self.angle_start) / (self.ticks - 1)
        for i in range(1, self.ticks):
            angle_full = self.angle_start + i * full_interval
            angle_half = angle_full - full_interval / 2

            self.drawTick(angle_full, RADIAL_TICK_RATIO_LARGE, WIDTH_LARGE, 
                          i * self.value_max // (self.ticks - 1))
            self.drawTick(angle_half, RADIAL_TICK_RATIO_SMALL, WIDTH_SMALL, None)
    
    def drawTick(self, angle: int, length_ratio: float, width: int, 
                 label_value: int) -> None:
        rads = radians(angle)
        dist_x = self.radius * cos(rads)
        dist_y = self.radius * sin(rads)
        
        start_pos = [self.center_x + dist_x, self.center_y + dist_y]
        end_pos = [self.center_x + dist_x * (1 - length_ratio), 
                   self.center_y + dist_y * (1 - length_ratio)]
        
        Color(*COLOUR_TICKS)
        Line(points = start_pos + end_pos, width = dp(width))
        
        if label_value is not None:
            size = dp(40)
            label = Label(text = str(label_value), font_size = TICK_LABEL,
                          color = COLOUR_TICKS, halign = 'center', 
                          valign = 'middle', size = (size, size),
                          center = (end_pos[0] - size * cos(rads) / 2, 
                                    end_pos[1] - size * sin(rads) / 2))
            self.add_widget(label)
