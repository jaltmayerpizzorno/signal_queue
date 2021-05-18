#define PY_SSIZE_T_CLEAN    // programmers love obscure statements
#include <Python.h>

#include <deque>
#include <mutex>


struct SimpleQueueObject {
  PyObject_HEAD
  std::mutex _m;
  std::condition_variable _cv;
  std::deque<PyObject*> _queue;
};


static PyObject*
SimpleQueue_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
  SimpleQueueObject* self = (SimpleQueueObject*) type->tp_alloc(type, 0);
  if (self != nullptr) {
    new (self) SimpleQueueObject;
  }

  return (PyObject*)self;
}


static void
SimpleQueue_dealloc(SimpleQueueObject* self) {
  self->~SimpleQueueObject();
  Py_TYPE(self)->tp_free((PyObject*)self);
}


static PyObject*
SimpleQueue_qsize(SimpleQueueObject* self, PyObject* Py_UNUSED(ignored)) {
  std::lock_guard<decltype(self->_m)> g(self->_m);
  return PyLong_FromLong(self->_queue.size());
}


static PyObject*
SimpleQueue_empty(SimpleQueueObject* self, PyObject* Py_UNUSED(ignored)) {
  std::lock_guard<decltype(self->_m)> g(self->_m);
  return PyBool_FromLong(self->_queue.size() == 0);
}


static PyObject*
SimpleQueue_put(SimpleQueueObject* self, PyObject* item) {
  Py_INCREF(item);

  // XXX add (but ignore) block=, timeout=

  {
    std::lock_guard<decltype(self->_m)> g(self->_m);
    self->_queue.push_back(item);
  }
  self->_cv.notify_one();

  Py_RETURN_NONE;
}


static PyObject*
SimpleQueue_get(SimpleQueueObject* self, PyObject* Py_UNUSED(ignored)) {
  PyObject* item = nullptr;

  // XXX add support for block=, timeout=

  Py_BEGIN_ALLOW_THREADS;

  {
    std::unique_lock<decltype(self->_m)> g(self->_m);

    self->_cv.wait(g, [self]()->bool {return !self->_queue.empty();});
    item = self->_queue.front();
    self->_queue.pop_front();
  }

  Py_END_ALLOW_THREADS;

  return item;
}


static PyMethodDef SimpleQueue_methods[] = {
  {"qsize", (PyCFunction)SimpleQueue_qsize, METH_NOARGS, "Returns this queue's size"},
  {"empty", (PyCFunction)SimpleQueue_empty, METH_NOARGS, "Returns whether this queue is empty"},
  {"put", (PyCFunction)SimpleQueue_put, METH_O, "Adds an item to this queue's tail"},
  // XXX add put_nowait
  {"get", (PyCFunction)SimpleQueue_get, METH_NOARGS, "Retrieves an item from this queue's head"},
  // XXX add get_nowait
  {NULL}
};


static PyTypeObject SimpleQueueType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "signal_queue.SimpleQueue",
  .tp_doc = "Signal handler safe queue",
  .tp_basicsize = sizeof(SimpleQueueObject),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_new = SimpleQueue_new,
  .tp_dealloc = (destructor) SimpleQueue_dealloc,
  .tp_methods = SimpleQueue_methods,
};


static struct PyModuleDef signal_queue_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "signal_queue",
    .m_doc = NULL, // no documentation
    .m_size = -1,
};


PyMODINIT_FUNC
PyInit_signal_queue() {
  if (PyType_Ready(&SimpleQueueType) < 0) {
    return nullptr;
  }

  PyObject* m = PyModule_Create(&signal_queue_module);
  if (m == nullptr) {
    return nullptr;
  }

  Py_INCREF(&SimpleQueueType);
  if (PyModule_AddObject(m, "SimpleQueue", (PyObject*) &SimpleQueueType) < 0) {
    Py_DECREF(&SimpleQueueType);
    Py_DECREF(m);
    return nullptr;
  }

  return m;
}
