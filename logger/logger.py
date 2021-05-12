#!/usr/bin/python3

# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 12.05.2021
# Autor: Marcin Białek


import os
import re
import csv
import time
import traceback
from argparse import ArgumentParser


cpu_time_re = re.compile(r"cpu" + 10 * r"\s+(\d+)") 
proc_stat_re = re.compile(r"(?:\S+ ){13}(\d+) (\d+) (?:\S+ ){7}(\d+)(?: \S+){29}")


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-p", "--period", type=int, help="period in ms (default 1000)")
    parser.add_argument("-l", "--logdir", type=str, help="log directory")
    parser.add_argument("pids", type=int, nargs="+", help="process pids")
    args = parser.parse_args()
    period = args.period if (args.period and args.period > 0) else 1000
    period = period / 1000
    logdir = args.logdir or os.getcwd()

    if any(p <= 0 for p in args.pids):
        print("invalid pids")
        exit(1)

    try:
        last_total_time = 0
        stat_file = open("/proc/stat")
        procs = []

        for pid in args.pids:
            proc_stat_file = open(f"/proc/{pid}/stat")
            log_file = open(os.path.join(logdir, f"log_{pid}.csv"), "w")
            csv_writer = csv.writer(log_file)
            csv_writer.writerow(("time", "memory", "cpu_usage"))
            procs.append({
                "pid": pid,
                "last_time": 0,
                "stat_file": proc_stat_file,
                "log_file": log_file,
                "csv_writer": csv_writer
            })

        while True:
            total_time = sum([int(t) for t in cpu_time_re.match(stat_file.read()).groups()])
            dt = total_time - last_total_time
            stat_file.seek(0)
            current_time = int(1000000 * time.time())

            for proc in procs:
                g = proc_stat_re.match(proc["stat_file"].read()).groups()
                proc["stat_file"].seek(0)
                t = int(g[0]) + int(g[1])
                m = int(g[2])

                if proc["last_time"] > 0 and last_total_time > 0:
                    u = (t - proc["last_time"]) / dt 
                    proc["csv_writer"].writerow((current_time, m, u))

                proc["last_time"] = t 
                
            last_total_time = total_time
            time.sleep(period)

    except KeyboardInterrupt:
        ...
    except Exception:
        traceback.print_exc()
    finally:
        stat_file.close()

        for proc in procs:
            proc["stat_file"].close()
            proc["log_file"].close() 
