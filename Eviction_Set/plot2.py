# import matplotlib.pyplot as plt

# def plot_data(file_name):
    # # Lese die Daten aus der Datei
    # with open(file_name, 'r') as file:
        # data = [line.strip().split() for line in file]

    # # Extrahiere x- und y-Werte
    # x_values = [int(row[0]) for row in data]
    # y_values = [int(row[1]) for row in data]

    # # Erstelle den Plot
    # plt.plot(x_values, y_values, marker='o', linestyle='-')
    # plt.xlabel('Anzahl')
    # plt.ylabel('Cycles')
    # plt.title('Plot der Daten aus {}'.format(file_name))
    # plt.grid(True)
    # plt.show()

# # Beispielaufruf des Skripts mit einer Datei namens 'data.txt'
# file_name = input("Geben Sie den Dateinamen ein: ")
# plot_data(file_name)

# import matplotlib.pyplot as plt

# def plot_data(file_name):
    # # Lese die Daten aus der Datei
    # with open(file_name, 'r') as file:
        # data = [line.strip().split() for line in file]

    # # Extrahiere x- und y-Werte
    # x_values = [int(row[0]) for row in data]
    # y_values = [int(row[1]) for row in data]

    # # Erstelle den Scatterplot
    # plt.scatter(x_values, y_values, marker='o', color='blue')
    # plt.xlabel('Anzahl Elemente')
    # plt.ylabel('Anzahl Cycles')
    # plt.title('Scatterplot der Daten aus {}'.format(file_name))
    # plt.grid(True)
    # plt.show()

# # Beispielaufruf des Skripts mit einer Datei namens 'data.txt'
# file_name = input("Geben Sie den Dateinamen ein: ")
# plot_data(file_name)

import matplotlib.pyplot as plt
import numpy as np

def plot_data(file_name):
    # Read the data from the file
    with open(file_name, 'r') as file:
        data = [line.strip().split() for line in file]

    # Extract x- and y-values
    x_values = np.array([int(row[0]) for row in data])
    y_values = np.array([int(row[1]) for row in data])

    # Find unique integer values on x-axis
    unique_x = np.unique(x_values)

    # Create a boxplot for each integer value on the x-axis
    boxplot_data = [y_values[x_values == x] for x in unique_x]
    plt.boxplot(boxplot_data, labels=unique_x)

    plt.xlabel('Anzahl Elemente')
    plt.ylabel('Anzahl Cycles')
    plt.title('Boxplot der Daten aus {}'.format(file_name))
    plt.grid(True)
    plt.show()

# Example call of the script with a file named 'data.txt'
file_name = input("Geben Sie den Dateinamen ein: ")
plot_data(file_name)
