import matplotlib.pyplot as plt

def plot_data(file_name):
    # Lese die Daten aus der Datei
    with open(file_name, 'r') as file:
        data = [line.strip().split() for line in file]

    # Extrahiere x- und y-Werte
    x_values = [int(row[0]) for row in data]
    y_values = [int(row[1]) for row in data]

    # Erstelle den Plot
    plt.plot(x_values, y_values, marker='o', linestyle='-')
    plt.xlabel('Anzahl')
    plt.ylabel('Cycles')
    plt.title('Plot der Daten aus {}'.format(file_name))
    plt.grid(True)
    plt.show()

# Beispielaufruf des Skripts mit einer Datei namens 'data.txt'
file_name = input("Geben Sie den Dateinamen ein: ")
plot_data(file_name)
