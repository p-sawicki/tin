import csv
import numpy
import matplotlib.pyplot as plt
from datetime import datetime


def readFile(filename):
    """
    reading csv file
    file format: time, memory_usage, cpu_usage
    """
    with open(filename, newline='') as csvfile:
        reader = csv.reader(csvfile)

        for row in reader:
            time_t.append(int(row[0]))
            mem_usage.append(int(row[1]))
            cpu_usage.append(int(row[2]))

    return time_t, mem_usage, cpu_usage


def plot(xlabel, ylabel, x, y):
    """
    drawing and saving plot
    """
    act_time = datetime.now().strftime("%H:%M:%S")

    average = numpy.average(y)
    std_deviation = numpy.std(y, ddof=1)

    plt.plot(x, y)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)

    plt.title(f'Wykres {ylabel} - {xlabel}\n'
              f'Średnia: {average:.2f} '
              f'Odchylenie standardowe: {std_deviation:.2f}')
    plt.savefig(f'{act_time}_{ylabel}_{xlabel}.jpg', dpi=72)
    plt.clf()


if __name__ == '__main__':

    time_t = []
    mem_usage = []
    cpu_usage = []

    time_t, mem_usage, cpu_usage = readFile('stat.csv')

    plot('czas', 'zajętość pamięci', time_t, mem_usage)
    plot('czas', 'zużycie procesora', time_t, cpu_usage)
