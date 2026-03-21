import pandas as pd
import matplotlib.pyplot as plt

def plot_car_telemetry(csv_filename):
    # 1. Load the data
    # pandas handles the empty trailing commas (,,) by automatically assigning NaN
    df = pd.read_csv(csv_filename)

    # 2. Convert Unix timestamp to relative time (seconds starting from 0)
    # This makes the X-axis much easier to read
    df['Time_s'] = (df['Timestamp'] - df['Timestamp'].iloc[0]) / 1000.0

    # 3. Create a 2x2 grid of subplots
    fig, axs = plt.subplots(2, 2, figsize=(16, 10))
    fig.suptitle('Car Telemetry Log Visualization', fontsize=18, fontweight='bold')

    # --- TOP LEFT: Speed and RPM ---
    # We use a "twinx" axis because Speed and RPM have very different scales
    ax1 = axs[0, 0]
    color1 = 'tab:blue'
    ax1.set_xlabel('Time (s)')
    ax1.set_ylabel('Speed', color=color1, fontweight='bold')
    ax1.plot(df['Time_s'], df['Speed'], color=color1, label='Speed')
    ax1.tick_params(axis='y', labelcolor=color1)
    ax1.grid(True, alpha=0.3)

    ax1_twin = ax1.twinx()
    color2 = 'tab:red'
    ax1_twin.set_ylabel('RPM', color=color2, fontweight='bold')
    ax1_twin.plot(df['Time_s'], df['RPM'], color=color2, alpha=0.7, label='RPM')
    ax1_twin.tick_params(axis='y', labelcolor=color2)
    ax1.set_title('Speed & RPM over Time')

    # --- TOP RIGHT: Suspension Stroke ---
    ax2 = axs[0, 1]
    ax2.plot(df['Time_s'], df['FL_Susp'], label='Front Left', alpha=0.8)
    ax2.plot(df['Time_s'], df['FR_Susp'], label='Front Right', alpha=0.8)
    ax2.plot(df['Time_s'], df['RL_Susp'], label='Rear Left', alpha=0.8)
    ax2.plot(df['Time_s'], df['RR_Susp'], label='Rear Right', alpha=0.8)
    ax2.set_xlabel('Time (s)')
    ax2.set_ylabel('Suspension Travel')
    ax2.set_title('Suspension Stroke (All 4 Corners)')
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    # --- BOTTOM LEFT: Engine Temp & Fuel ---
    # Another twin axis for differing scales
    ax3 = axs[1, 0]
    ax3.plot(df['Time_s'], df['Temp'], color='orange', label='Temperature', linewidth=2)
    ax3.set_xlabel('Time (s)')
    ax3.set_ylabel('Temperature', color='orange', fontweight='bold')
    ax3.tick_params(axis='y', labelcolor='orange')
    ax3.grid(True, alpha=0.3)

    ax3_twin = ax3.twinx()
    ax3_twin.plot(df['Time_s'], df['Fuel'], color='green', label='Fuel Level', linewidth=2)
    ax3_twin.set_ylabel('Fuel', color='green', fontweight='bold')
    ax3_twin.tick_params(axis='y', labelcolor='green')
    ax3.set_title('Engine Temp & Fuel Level')

    # --- BOTTOM RIGHT: Z-Acceleration (Vertical) ---
    # Z-Accel is extremely useful for identifying bumps, track anomalies, or harsh landings
    ax4 = axs[1, 1]
    ax4.plot(df['Time_s'], df['FL_Accel_Z'], label='Front Left Z', alpha=0.7)
    ax4.plot(df['Time_s'], df['FR_Accel_Z'], label='Front Right Z', alpha=0.7)
    ax4.plot(df['Time_s'], df['RL_Accel_Z'], label='Rear Left Z', alpha=0.7)
    ax4.plot(df['Time_s'], df['RR_Accel_Z'], label='Rear Right Z', alpha=0.7)
    ax4.set_xlabel('Time (s)')
    ax4.set_ylabel('Acceleration (Z)')
    ax4.set_title('Vertical Acceleration (Bumps/Impacts)')
    ax4.legend()
    ax4.grid(True, alpha=0.3)

    # Adjust layout so labels don't overlap, then display
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    # Change this to the exact name of your car's log file
    plot_car_telemetry('baja_log_20260207_0945.csv')

    # helpful logs:
    # baja_log_20260207_0836.csv
    # baja_log_20260207_0940.csv