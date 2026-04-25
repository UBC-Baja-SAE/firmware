import pandas as pd
import matplotlib.pyplot as plt

def plot_car_telemetry(csv_filename):
    # 1. Load the data
    df = pd.read_csv(csv_filename)

    # 2. Convert Unix timestamp to relative time (seconds starting from 0)
    df['Time_s'] = (df['Timestamp'] - df['Timestamp'].iloc[0]) / 1000.0

    # 3. Create a 3x2 grid of subplots and increase the figure height (16, 15)
    fig, axs = plt.subplots(3, 2, figsize=(12, 11))
    fig.suptitle('Car Telemetry Log Visualization', fontsize=14, fontweight='bold')

    # --- TOP LEFT: Speed and RPM ---
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

    # --- MIDDLE LEFT: Engine Temp & Fuel ---
    # ax3 = axs[1, 0]
    # ax3.plot(df['Time_s'], df['Temp'], color='orange', label='Temperature', linewidth=2)
    # ax3.set_xlabel('Time (s)')
    # ax3.set_ylabel('Temperature', color='orange', fontweight='bold')
    # ax3.tick_params(axis='y', labelcolor='orange')
    # ax3.grid(True, alpha=0.3)

    # ax3_twin = ax3.twinx()
    # ax3_twin.plot(df['Time_s'], df['Fuel'], color='green', label='Fuel Level', linewidth=2)
    # ax3_twin.set_ylabel('Fuel', color='green', fontweight='bold')
    # ax3_twin.tick_params(axis='y', labelcolor='green')
    # ax3.set_title('Engine Temp & Fuel Level')

    # --- MIDDLE RIGHT: Z-Acceleration (Vertical) ---
    ax4 = axs[1, 1]
    ax4.plot(df['Time_s'], df['FL_Accel_Z'], label='Front Left Z', alpha=0.7)
    ax4.plot(df['Time_s'], df['FR_Accel_Z'], label='Front Right Z', alpha=0.7)
    ax4.plot(df['Time_s'], df['RL_Accel_Z'], label='Rear Left Z', alpha=0.7)
    ax4.plot(df['Time_s'], df['RR_Accel_Z'], label='Rear Right Z', alpha=0.7)
    ax4.set_xlabel('Time (s)')
    ax4.set_ylabel('Acceleration (Z)')
    ax4.set_title('Vertical Acceleration')
    ax4.legend()
    ax4.grid(True, alpha=0.3)

    # --- BOTTOM LEFT: X-Acceleration (Longitudinal) ---
    ax5 = axs[2, 0]
    ax5.plot(df['Time_s'], df['FL_Accel_X'], label='Front Left X', alpha=0.7)
    ax5.plot(df['Time_s'], df['FR_Accel_X'], label='Front Right X', alpha=0.7)
    ax5.plot(df['Time_s'], df['RL_Accel_X'], label='Rear Left X', alpha=0.7)
    ax5.plot(df['Time_s'], df['RR_Accel_X'], label='Rear Right X', alpha=0.7)
    ax5.set_xlabel('Time (s)')
    ax5.set_ylabel('Acceleration (X)')
    ax5.set_title('Longitudinal Accel')
    ax5.legend()
    ax5.grid(True, alpha=0.3)

    # --- BOTTOM RIGHT: Y-Acceleration (Lateral) ---
    #ax6 = axs[2, 1]
    ax6 = axs[1, 0]
    ax6.plot(df['Time_s'], df['FL_Accel_Y'], label='Front Left Y', alpha=0.7)
    ax6.plot(df['Time_s'], df['FR_Accel_Y'], label='Front Right Y', alpha=0.7)
    ax6.plot(df['Time_s'], df['RL_Accel_Y'], label='Rear Left Y', alpha=0.7)
    ax6.plot(df['Time_s'], df['RR_Accel_Y'], label='Rear Right Y', alpha=0.7)
    ax6.set_xlabel('Time (s)')
    ax6.set_ylabel('Acceleration (Y)')
    ax6.set_title('Lateral Accel')
    ax6.legend()
    ax6.grid(True, alpha=0.3)

    # Adjust layout so labels don't overlap, then display
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    # Change this to the exact name of your car's log file
    plot_car_telemetry('baja_log_20260207_1305.csv')

    # helpful logs:
    # baja_log_20260207_0836.csv
    # baja_log_20260207_0940.csv
    # baja_log_20260207_0945.csv
    # baja_log_20260207_1006.csv