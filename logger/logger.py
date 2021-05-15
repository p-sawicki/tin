#!/usr/bin/python3

# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 12.05.2021
# Autor: Marcin Białek


import os
import re
import csv
import time
import queue
import traceback
import threading
from argparse import ArgumentParser
from ipcqueue import posixmq
from ipcqueue.serializers import RawSerializer


running = True
cpu_time_re = re.compile(r"cpu" + 10 * r"\s+(\d+)") 
proc_stat_re = re.compile(r"(?:\S+ ){13}(\d+) (\d+) (?:\S+ ){7}(\d+)(?: \S+){29}")


def logger_worker(proc_stack_lock: threading.Lock, proc_stack: list, period):
    try:
        last_total_time = 0
        stat_file = open("/proc/stat")

        while running:
            total_time = sum([int(t) for t in cpu_time_re.match(stat_file.read()).groups()])
            dt = total_time - last_total_time
            stat_file.seek(0)
            current_time = int(1000000 * time.time())
            proc_stack_lock.acquire(True)

            for proc in proc_stack:
                try:
                    g = proc_stat_re.match(proc["stat_file"].read()).groups()
                    proc["stat_file"].seek(0)
                    t = int(g[0]) + int(g[1])
                    m = int(g[2])

                    if proc["last_time"] > 0 and last_total_time > 0:
                        u = (t - proc["last_time"]) / dt 
                        proc["csv_writer"].writerow((current_time, m, u))

                    proc["last_time"] = t 
                except ProcessLookupError:
                    pass
                
            proc_stack_lock.release()
            last_total_time = total_time
            time.sleep(period)

    except Exception:
        traceback.print_exc()

    finally:
        stat_file.close()


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-p", "--period", type=int, help="period in ms (default 1000)")
    parser.add_argument("-l", "--logdir", type=str, help="log directory")
    parser.add_argument("inqueue", type=str, help="input queue")
    parser.add_argument("outqueue", type=str, help="output queue")
    args = parser.parse_args()
    period = args.period / 1000 if (args.period and args.period > 0) else 1
    logdir = args.logdir or os.getcwd()

    try:
        input_queue = posixmq.Queue(args.inqueue, serializer = RawSerializer)
        output_queue = posixmq.Queue(args.outqueue, serializer = RawSerializer)
        proc_stack_lock = threading.Lock()
        proc_stack = []
        worker = threading.Thread(target = logger_worker, args = (proc_stack_lock, proc_stack, period))
        worker.start()

        while running:
            msg = int.from_bytes(input_queue.get(), 'little', signed = True)

            if msg >= 0:
                proc_stat_file = open(f"/proc/{msg}/stat")
                log_file = open(os.path.join(logdir, f"log_{msg}.csv"), "w")
                csv_writer = csv.writer(log_file)
                csv_writer.writerow(("time", "memory", "cpu_usage"))
                proc_stack_lock.acquire(True)
                proc_stack.append({
                    "pid": msg,
                    "last_time": 0,
                    "stat_file": proc_stat_file,
                    "log_file": log_file,
                    "csv_writer": csv_writer
                })
                proc_stack_lock.release()
                output_queue.put(msg.to_bytes(4, 'little', signed = True))
                # print(f"Added process with pid {msg}")

            elif msg == -1:
                proc_stack_lock.acquire(True)

                if len(proc_stack) > 0:
                    proc = proc_stack.pop()
                    proc_stack_lock.release()
                    proc["stat_file"].close()
                    proc["log_file"].close() 
                    output_queue.put(int(0).to_bytes(4, 'little', signed = True))
                    # print(f"Removed process with pid {proc['pid']}")
                
                else:
                    proc_stack_lock.release()

            elif msg == -2:
                running = False 
                worker.join()
                output_queue.put(int(0).to_bytes(4, 'little', signed= True))
                # print("End")

    except KeyboardInterrupt:
        running = False 
        worker.join()

    except Exception:
        traceback.print_exc()

    finally:
        input_queue.close()
        output_queue.close()

        for proc in proc_stack:
            proc["stat_file"].close()
            proc["log_file"].close() 
