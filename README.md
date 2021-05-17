# signal_queue

__Work-In-Progress__ -- some key functionality is still missing.

Python C++ extension that replicates some functionality from `queue.Queue`, but is safe
to use within a Python signal handler.

If you attempt to use `queue.Queue` from within a (Python) signal handler, you will
likely run into a deadlock as it uses a `threading.Lock` and the signal may be delivered
a second time after that lock has been acquired, and thus attempt to acquire the lock
again.

Starting a thread from a signal handler has the same issue, as starting it involves
acquiring a lock or two.

`signal_queue.Queue` makes use of the fact that Python signal handlers are only called
from within the interpreter (i.e., can't interrupt native code).

## How to Use

Import and instantiate `Queue` from `signal_queue` rather than from the standard `queue`,
and use it in your signal handler:

Instead of
```python
import threading, signal
from signal_queue import Queue

q = Queue()

def handle_signal(signum, stack):
    q.put(stack)

def handler_thread():
    while True:
        item = q.get()
        ...
        q.task_done()

...
signal.signal(signal.SIGUSR1, handle_signal)
threading.Thread(target=handler_thread, daemon=True).start()
...
```
