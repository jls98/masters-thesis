import argparse
import numpy as np
import matplotlib.pyplot as plt

def parse_input_file(file_path):
    data = []
    with open(file_path, 'r') as file:
        for line in file:
            if line.strip():  # Ignore empty lines
                stride, cycle = map(float, line.split())
                data.append((stride, cycle))
    return np.array(data)

def plot_data(data):
    strides, cycles = data[:, 0], data[:, 1]
    
    plt.figure(figsize=(10, 6))
    plt.scatter(strides, cycles, label='cycles', marker='o', color='b')
    plt.yscale('linear')  # Adjust the scale if needed
    plt.xscale('log')
    
    plt.title('Cycle Count vs. Stride')
    plt.xlabel('Stride')
    plt.ylabel('Cycles')
    plt.grid(True)
    plt.legend()
    
    plt.show()

def main():
    parser = argparse.ArgumentParser(description='Plot data from input file.')
    parser.add_argument('input', help='Input file containing stride-cycle pairs.')
    args = parser.parse_args()

    input_file_path = args.input
    data = parse_input_file(input_file_path)
    plot_data(data)

if __name__ == "__main__":
    main()
