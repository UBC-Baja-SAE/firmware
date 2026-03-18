import sys
import os
import tkinter as tk
import random
import time

HEBREW_WORDS = [
    "שלום", "תודה", "מה נשמע", "מוות", "זמן", "דם", "לילה", 
    "הם צופים", "אין לאן לברוח", "סוף", "צל", "שתיקה", "רעב"
]
    

def trigger_scare(target_name):
    if "DISPLAY" not in os.environ:
        os.environ["DISPLAY"] = ":0"


    time.sleep(1)
    root = tk.Tk()
    root.attributes("-fullscreen", True)
    root.attributes("-topmost", True)
    root.configure(background='black')
 
    # Random time calculation
    minutes = random.randint(4, 49)
    death_msg = f"{target_name}"    
    # Label with a "handwritten" look
    # Note: On Pi, 'Chalkboard SE' or 'Purisa' are often available creepy-ish fonts
    label = tk.Label(root, text=death_msg, fg='white', bg='black', 
                     font=('Bogens', 60, 'bold'), wraplength=1000)
    label.pack(expand=True)

    # Flashing logic
    # for _ in range(7):
    #     # Flashing logic with jitter
    for _ in range(7):
        # Randomly offset the text slightly to look "unstable"
        label.place(relx=0.5 + random.uniform(-0.03, 0.03), 
                    rely=0.5 + random.uniform(-0.03, 0.03), 
                    anchor="center")
        label.config(fg='black')
        root.update()
        time.sleep(0.025)
        label.config(fg='white')
        root.update()
        time.sleep(5)
        label.config(fg='black')
        root.update()
        time.sleep(5)
    
    root.destroy()

if __name__ == "__main__":
    name = sys.argv[1] if len(sys.argv) > 1 else "You"
    trigger_scare(name)
