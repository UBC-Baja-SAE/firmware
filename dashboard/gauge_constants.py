
'''
The size of the screen being used, measured in pixels.
'''
SCREEN_SIZE = (1280, 400)

'''
Logo icon values
'''
LOGO_CENTER_Y = 300
LOGO_SIZE = 180
LOGO_PATH = './assets/images/logo.png'

'''
Common values for the radial gauges
'''
RADIAL_START_ANGLE = 195
RADIAL_END_ANGLE = -15
RADIUS = 200
RADIAL_SIZE = (400, 400)

'''
Speedometer values
'''
SPEEDOMETER_MAX_VALUE = 70
SPEEDOMETER_TICKS = 8
SPEEDOMETER_LABEL = "km/h"
SPEEDOMETER_SHOW_LABEL = True
SPEEDOMETER_CENTER_POSITION = (400, 120)

'''
Tachometer values
'''
TACHOMETER_MAX_VALUE = 5
TACHOMETER_TICKS = 6
TACHOMETER_LABEL = "x1000 RPM"
TACHOMETER_SHOW_LABEL = False
TACHOMETER_CENTER_POSITION = (880, 120)

'''
Common values for the linear gauges
'''
LINEAR_LENGTH = 200
LINEAR_BAR_WIDTH = 25
LINEAR_SIZE = (40, 200)
LINEAR_ICON_SIZE = 20

'''
Fuel gauge values
'''
FUEL_MAX_VALUE = 150
FUEL_TICKS = 2
FUEL_SYMBOL_MIN = 'E'
FUEL_SYMBOL_MAX = 'F'
FUEL_CENTER_POSITION = (50, 125)
FUEL_ICON_SIZE = 20
FUEL_ICON_PATH = './assets/images/fuel.png'

'''
Temperature gauge values
'''
TEMPERATURE_MAX_VALUE = 150
TEMPERATURE_TICKS = 2
TEMPERATURE_SYMBOL_MIN = 'C'
TEMPERATURE_SYMBOL_MAX = 'H'
TEMPERATURE_CENTER_POSITION = (125, 125)
TEMPERATURE_ICON_SIZE = 20
TEMPERATURE_ICON_PATH = './assets/images/temp.png'

'''
The colours utilised for the dashboard, written in RGBA format.
'''
COLOUR_PRIMARY = (95 / 256, 37 / 256, 159 / 256, 240 / 256)
COLOUR_BACKGROUND = (0, 0, 0, 1)
COLOUR_TICKS = (1, 1, 1, 1)
COLOUR_GRADIENT = (0.6, 0.6, 0.6)

'''
These widths correspond to the thicknesses of the tick marks and radial/linear
outline for the gauges.
'''
WIDTH_SMALL = 1.5
WIDTH_LARGE = 2

'''
The percentage of the radial gauge's radius that is taken up by the tick mark at
an angle. The major increments utilise RATIO_LARGE, and are an 1/8th (0.125) of
the radius, and so on.
'''
RADIAL_TICK_RATIO_LARGE = 0.125
RADIAL_TICK_RATIO_SMALL = 0.0625

'''
The percentage of the radial gauge's radius that has a gradient 
fill.
'''
RATIO_GRADIENT = 0.2

'''
The percentage of the radial gauge's radius that is filled, corresponding to the
gauge measurement.
'''
RATIO_FILL = 0.40

'''
The percentage of the linear gauge's bar width that is taken up by a small tick
mark
'''
RATIO_MINOR = 0.6

'''
These are the font sizes for the various labels in the gauge
'''
TICK_LABEL = '30sp'
GAUGE_LABEL = '150sp'
LINEAR_GAUGE_LABEL = '25sp'