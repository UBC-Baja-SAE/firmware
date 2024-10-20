class Gaugeable:
    def __init__(self,  angle_start: int, angle_end: int, value_max: int,
                 radius: int, **kwargs):
        super().__init__(**kwargs)
        self.angle_start = angle_start
        self.angle_end = angle_end
        self.value_max = value_max
        self.radius = radius
        
    """
    Converts a value into the absolute angle based on the maximum value allowed
    @param value
    @returns    a value 'angle' such that angle_end <= angle <= angle_start
    """
    def getAngle(self, value: int) -> int:
        value_percentage = value / self.value_max
        angle_range = self.angle_start - self.angle_end
        
        return self.angle_start - angle_range * value_percentage