import csv
import numpy
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from datetime import datetime
from argparse import ArgumentParser
from itertools import zip_longest


def readFile(filename):
    """
    reading csv file
    file format: time, memory_usage, cpu_usage
    """
    with open(filename, newline='') as csvfile:
        reader = csv.reader(csvfile)
        next(reader) # skip header
        time_t.append([])
        mem_usage.append([])
        cpu_usage.append([])

        for row in reader:
            time_t[-1].append(int(row[0]))
            mem_usage[-1].append(int(row[1]))
            cpu_usage[-1].append(float(row[2]))


def plot(name, xlabel, ylabel, x, y):
    """
    drawing and saving plot
    """
    act_time = datetime.now().strftime("%H:%M:%S")

    average = numpy.average(y)
    std_deviation = numpy.std(y, ddof=1)

    plt.plot(x, y)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)

    plt.title(f'{name}\n'
              f'Średnia: {average:.2f} '
              f'Odchylenie standardowe: {std_deviation:.2f}')
    plt.savefig(f'{act_time}_{name}.jpg', dpi=72)
    plt.clf()


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("log_file", nargs="+", type=str, help="log files")
    parser.add_argument("scenario", type=str, help="scenario name")
    args = parser.parse_args()
    time_t = []
    mem_usage = []
    cpu_usage = []
    
    for filename in args.log_file:
        readFile(filename)

    time_t = max(time_t, key=len)
    time_t = [(t - time_t[0])/1000 for t in time_t]
    mem_usage = [(sum(v)/len(v))/1000000 for v in zip_longest(*mem_usage, fillvalue=0)]
    cpu_usage = [sum(v)/len(v) for v in zip_longest(*cpu_usage, fillvalue=0)]
    plot(f'{args.scenario}: średnie zużycie pamięci', 'czas [s]', 'zajętość pamięci [MB]', time_t, mem_usage)
    plot(f'{args.scenario}: średnie zużycie procesora', 'czas [s]', 'zużycie procesora [%]', time_t, cpu_usage)
