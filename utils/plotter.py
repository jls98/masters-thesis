import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

def parse_input_file(file_path):
    data = []
    with open(file_path, 'r') as file:
        for line in file:
            if line.strip():  # Ignore empty lines
                stride, cycle = map(float, line.split())
                data.append((stride, cycle))
    return np.array(data)

def format_ticks(value, _):
    # Custom tick formatter to display powers of 2
    return f'2^{int(np.log2(value))}'
    
def plot_data_task2(data):
    strides, cycles = data[:, 0], data[:, 1]
    
    plt.figure(figsize=(10, 6))
    plt.scatter(strides, cycles, label='cycles', marker='o', color='b')
    plt.plot(strides, cycles, linestyle='-', color='b', linewidth=0.5)  # Thin line connecting the dots
    plt.yscale('linear')  # Adjust the scale if needed
    plt.xscale('log', base=2)
    
    plt.title('Cycle Count per Stride')
    plt.xlabel('Stride')
    plt.ylabel('Cycles')
    
    # Ensure a minimum number of ticks on the x-axis
    plt.gca().xaxis.set_major_formatter(FuncFormatter(format_ticks))
    
    plt.grid(True)
    plt.legend()
    
    plt.show()
    
def plot_data_task1(data):
    strides, cycles = data[:, 0], data[:, 1]
    
    plt.figure(figsize=(10, 6))
    plt.scatter(strides, cycles, label='cycles', marker='o', color='b')
    plt.plot(strides, cycles, linestyle='-', color='b', linewidth=0.5)  # Thin line connecting the dots
    plt.yscale('linear')  # Adjust the scale if needed
    plt.xscale('log', base=2)
    
    plt.title('Cycle Count by Memsize')
    plt.xlabel('Memsize')
    plt.ylabel('Cycles')
    
    # Ensure a minimum number of ticks on the x-axis
    plt.gca().xaxis.set_major_formatter(FuncFormatter(format_ticks))
    
    plt.grid(True)
    plt.legend()
    
    plt.show()

def main():
    parser = argparse.ArgumentParser(description='Plot data from input file.')
    parser.add_argument('filename', help='Input file containing stride-cycle pairs.')
    parser.add_argument('task', help='task to plot.')
    args = parser.parse_args()
    
    # put inputs in vars
    input_file_path = args.filename
    current_task = args.task
    
    # parse data from file
    data = parse_input_file(input_file_path)
    
    # select task
    if current_task == 'task1':
        plot_data_task1(data)
    elif current_task == 'task2':
        plot_data_task2(data)

         
        
    

if __name__ == "__main__":
    main()
