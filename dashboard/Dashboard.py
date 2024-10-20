from kivy.app import App
from kivy.uix.relativelayout import RelativeLayout
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.widget import Widget
from kivy.clock import Clock
from kivy.core.window import Window
from kivy.lang import Builder
from RadialGauge.Gauge import Gauge
from LinearGauge.LinearGauge import LinearGauge
from math import cos, sin

Window.size = (1280, 400)

Builder.load_file("./assets/layout/logo.kv")
Builder.load_file("./assets/layout/fuel.kv")
Builder.load_file("./assets/layout/temp.kv")

class Fuel(Widget):
    pass

class Temp(Widget):
    pass

class Logo(Widget):
    pass
    
class DashboardApp(App):
    def __init__(self, **kwargs):

        super(DashboardApp, self).__init__(**kwargs)
        
        self.time = 0

    def build(self):
        self.layout = FloatLayout(size = (1280, 400))
        
        self.layout.add_widget(Logo())
        
        self.speedometer = Gauge(195, -15, 70, 200, 8, "km/h", True, size = (400, 400), center = (400, 120))
        self.tachometer = Gauge(195, -15, 5, 200, 6, "x1000 RPM", False, size = (400, 400), center = (880, 120))
        self.fuel = LinearGauge(150, 200, 40, 20, 2, 'E', 'F', size = (40, 200), center = (50, 125))
        self.fuel.add_widget(Fuel())
        self.temp = LinearGauge(150, 200, 40, 20, 2, 'C', 'H', size = (40, 200), center = (125, 125))
        self.temp.add_widget(Temp())
        
        self.layout.add_widget(self.temp)
        self.layout.add_widget(self.fuel)
        self.layout.add_widget(self.speedometer)
        self.layout.add_widget(self.tachometer)
        
        Clock.schedule_interval(self.simulateChange, 1 / 20)
        
        return self.layout
        
    def simulateChange(self, dt):
        speed = 70 / 2 * (1 + sin(self.time * 3))
        self.speedometer.update(speed)
        
        tach = 5 / 2 * (1 + cos(self.time * 2))
        self.tachometer.update(tach)
        
        temp = 150 / 2 * (1 + sin(self.time / 2))
        self.temp.update(temp)
        
        fuel = 150 / 2 * (1 + cos(self.time / 1.5))
        self.fuel.update(fuel)

        self.time += 1 / 20
    
if __name__ == '__main__':
    DashboardApp().run()


