import sys
import numpy as np
import matplotlib.pyplot as plt

def plot_data(file_path = data):
    # Lese Daten aus der Textdatei
    data = np.loadtxt(file_path)

    # Extrahiere Daten aus den Spalten
    strides = data[:, 0]
    cycles = data[:, 1]

    # Plot
    plt.figure(figsize=(10, 6))
    plt.scatter(strides, cycles, c=cycles, cmap='viridis', marker='o')
    plt.colorbar(label='Messwerte')
    plt.xscale('log')  # Logarithmische Skala auf der x-Achse
    plt.xlabel('Stride')
    plt.ylabel('Cycles')
    plt.title('Messungen')
    plt.show()

if __name__ == "__main__":
    # Überprüfe, ob der Dateipfad als Argument angegeben wurde
    if len(sys.argv) != 2:
        print("Verwendung: python script.py <input_file>")
        sys.exit(1)

    file_path = sys.argv[1]
    plot_data(file_path)
