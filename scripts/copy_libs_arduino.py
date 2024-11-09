import os
import shutil
import argparse
'''
Example use: from the root of this repository, use python scripts/copy_libs_arduino.py --destination=/c/Users/geoff/OneDrive/Documents/Arduino/libraries
'''

current_path = os.getcwd()
source = f"{current_path}/lib"

def copy_over_libs(source, destination):
    print("here")
    try:
        for item in os.listdir(source):
            print(item)

            source_path = os.path.join(source, item)
            destination_path = os.path.join(destination, item)

            print(source_path)
            print(destination_path)
            if os.path.isdir(source_path):
                if os.path.exists(destination_path):
                    shutil.rmtree(destination_path)
                
                shutil.copytree(source_path, destination_path)
                print("Gucci")
    except Exception as e:
        print("You've failed me Anakin")

def main():
    parser = argparse.ArgumentParser(description="Parsing arguments")
    parser.add_argument('--destination', type=str)
    args = parser.parse_args()
    copy_over_libs(source, args.destination)

if __name__ == "__main__":
    main()

