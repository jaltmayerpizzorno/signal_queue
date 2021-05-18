# signal_queue
[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](LICENSE) 

Python C++ extension that replicates (most of the) functionality from `queue.SimpleQueue`
(available in Python 3.7+) and is safe to use within a Python signal handler.

## Why it exists

I was looking for a way implement a producer-consumer queue for handling Python signals,
and didn't realize `queue.SimpleQueue` was already reentrant.

It turns out that if you attempt to use `queue.Queue` from within a (Python) signal handler,
a deadlock is likely to result: it uses a `threading.Lock` and a signal may be delivered
a second time after that lock has been acquired, causing it to attempt to acquire the lock again.

Starting a thread from a signal handler has the same issue, as starting it involves acquiring a
lock or two.

`signal_queue.SimpleQueue` makes use of the fact that Python signal handlers are only called
from within the interpreter (i.e., can't interrupt native code).

## How to Use

Pretty much like you'd use Python's `queue.SimpleQueue`:

```python
import threading, signal
from signal_queue import SimpleQueue

q = SimpleQueue()

def handle_signal(signum, stack):
    q.put(stack)

def handler_thread():
    while True:
        item = q.get()
        ...

...
signal.signal(signal.SIGUSR1, handle_signal)
threading.Thread(target=handler_thread, daemon=True).start()
...
```
