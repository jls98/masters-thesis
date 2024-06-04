import pandas as pd
import matplotlib.pyplot as plt

# Define the threshold
threshold = 110  # You can adjust this value as needed

# Read the file into a pandas DataFrame
filename = 'measurements'  # Replace with your file name
df = pd.read_csv(filename)

# Get the number of columns
num_columns = df.shape[1]

# Calculate the duration for a block
block_duration = (num_columns - 1) * 3000

# Function to count measurements below the threshold excluding consecutive ones
def count_below_threshold(df, threshold):
    counts = []
    for col in df.columns[1:]:
        below_threshold = df[col] < threshold
        non_consecutive_below_threshold = below_threshold & (~below_threshold.shift(1, fill_value=False))
        count = non_consecutive_below_threshold.sum()
        counts.append(count)
    return counts

# Function to find the first measurement below the threshold
def first_below_threshold(df, threshold):
    first_below = {}
    for col in df.columns[1:]:
        below_threshold = df[col] < threshold
        first_index = below_threshold.idxmax() if below_threshold.any() else None
        first_below[col] = first_index
    return first_below

# Count the measurements below the threshold
counts = count_below_threshold(df, threshold)
print("Counts of measurements below the threshold (excluding consecutive ones):")
for col, count in zip(df.columns[1:], counts):
    print(f"{col}: {count}")

# Find the first measurement below the threshold
first_below = first_below_threshold(df, threshold)
print("\nFirst measurements below the threshold:")
for col, index in first_below.items():
    if index is not None:
        print(f"{col}: Block {df.at[index, df.columns[0]]}, Index {index}")
    else:
        print(f"{col}: None below threshold")

# Plotting the measurements
plt.figure(figsize=(12, 6))
for col in df.columns[1:]:
    below_threshold_indices = df[df[col] < threshold].index
    if not below_threshold_indices.empty:
        start_idx = below_threshold_indices[0]
        end_idx = below_threshold_indices[-1]
        plt.plot(df[df.columns[0]][start_idx:end_idx+1], df[col][start_idx:end_idx+1], label=col)
plt.xlabel('Blocks')
plt.ylabel('Measurements')
plt.title('Measurements below the threshold')
plt.legend()
plt.show()