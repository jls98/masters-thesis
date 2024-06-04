import numpy as np
import matplotlib.pyplot as plt

# Define the threshold
threshold = 110

# Function to parse the file
def parse_file(filename):
    data = np.loadtxt(filename)
    return data

# Function to count measurements below the threshold
def count_below_threshold(data, threshold):
    counts = []
    for col in range(1, data.shape[1]):
        count = 0
        below_threshold = False
        for i in range(len(data)):
            if data[i, col] < threshold:
                if not below_threshold:
                    count += 1
                    below_threshold = True
            else:
                below_threshold = False
        counts.append(count)
    return counts

# Function to find the first measurement below the threshold
def first_below_threshold(data, threshold):
    first_occurrences = []
    for col in range(1, data.shape[1]):
        for i in range(len(data)):
            if data[i, col] < threshold:
                first_occurrences.append(i)
                break
    return first_occurrences

# Function to plot the measurements
def plot_measurements(data, threshold):
    for col in range(1, data.shape[1]):
        below_indices = np.where(data[:, col] < threshold)[0]
        if len(below_indices) > 0:
            start_index = max(0, below_indices[0] - 10)
            end_index = min(len(data), below_indices[-1] + 10)
            plt.plot(data[start_index:end_index, 0], data[start_index:end_index, col], label=f'Column {col}')
    plt.axhline(y=threshold, color='r', linestyle='--', label='Threshold')
    plt.xlabel('Blocks')
    plt.ylabel('Measurements')
    plt.legend()
    plt.show()

# Main script
filename = 'data.txt'
data = parse_file(filename)
counts = count_below_threshold(data, threshold)
first_occurrences = first_below_threshold(data, threshold)

print("Counts of measurements below threshold (excluding consecutive ones):", counts)
print("First occurrence of measurements below threshold in each column:", first_occurrences)

plot_measurements(data, threshold)
