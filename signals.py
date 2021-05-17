import signal, time, threading, queue, os
import signal_queue

#q = queue.Queue()
q = signal_queue.Queue()

count = 0;

def handle_signal(signum, stack):
    time.sleep(.0001)
    q.put(stack)

def worker():
    global count
    while True:
        item = q.get()
        q.task_done()
        count = count + 1

def interrupter():
    while True:
        time.sleep(.0001)
        os.kill(os.getpid(), signal.SIGUSR1)

def sizeTeller():
    while True:
        time.sleep(1)
        print("queue size:", q.qsize())

print('pid:', os.getpid())
signal.signal(signal.SIGUSR1, handle_signal)
threading.Thread(target=sizeTeller, daemon=True).start()
threading.Thread(target=interrupter, daemon=True).start()

threading.Thread(target=worker, daemon=True).start()

time.sleep(10)
print(count);
