class Gaugeable:
    def __init__(self,  angle_start: int, angle_end: int, value_max: int,
                 radius: int, **kwargs):
        super().__init__(**kwargs)
        self.angle_start = angle_start
        self.angle_end = angle_end
        self.value_max = value_max
        self.radius = radius
        
    def getAngle(self, value: int) -> int:
        """Gets the angle corresponding to the given value for this gauge.

        Args:
            value (int):    The new value. Must be >= 0.

        Returns:
            int:    An angle that corresponds to the given value for the gauge,
                    measured in degrees counter-clockwise from the right side of
                    the horizontal axis, where angle_start >= angle >= angle_end
        """
        # bound percentage between [0,1]
        value_percentage = min(max(value / self.value_max, 0), 1)
        
        angle_range = self.angle_start - self.angle_end
        
        return self.angle_start - angle_range * value_percentage