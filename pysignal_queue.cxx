#define PY_SSIZE_T_CLEAN    // programmers love obscure statements
#include <Python.h>

#include <deque>
#include <mutex>

//#include <iostream>


struct QueueObject {
  PyObject_HEAD
  std::mutex _m;
  std::condition_variable _cv;
  std::deque<PyObject*> _queue;
};


static PyObject*
Queue_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
  QueueObject* self = (QueueObject*) type->tp_alloc(type, 0);
  if (self != nullptr) {
    new (self) QueueObject;
  }

  return (PyObject*)self;
}


static void
Queue_dealloc(QueueObject* self) {
  self->~QueueObject();
  Py_TYPE(self)->tp_free((PyObject*)self);
}


static PyObject*
Queue_put(QueueObject* self, PyObject* item) {
  Py_INCREF(item);

  {
    std::lock_guard<decltype(self->_m)> g(self->_m);
    self->_queue.push_back(item);
  }
  self->_cv.notify_one();

  Py_RETURN_NONE;
}


static PyObject*
Queue_get(QueueObject* self, PyObject* Py_UNUSED(ignored)) {
  PyObject* item = nullptr;

  Py_BEGIN_ALLOW_THREADS;

  {
    std::unique_lock<decltype(self->_m)> g(self->_m);

    self->_cv.wait(g, [self]()->bool {return !self->_queue.empty();});
    item = self->_queue.front();
    self->_queue.pop_front();
  }

  Py_END_ALLOW_THREADS;

  // XXX don't need to drop a reference, right?

  return item;
}


static PyObject*
Queue_task_done(QueueObject* self, PyObject* Py_UNUSED(ignored)) {
  // XXX implement
  Py_RETURN_NONE;
}


static PyObject*
Queue_qsize(QueueObject* self, PyObject* Py_UNUSED(ignored)) {
  std::lock_guard<decltype(self->_m)> g(self->_m);
  return PyLong_FromLong(self->_queue.size());
}


static PyMethodDef Queue_methods[] = {
  {"put", (PyCFunction)Queue_put, METH_O, "Adds an item to this queue's tail"},
  {"get", (PyCFunction)Queue_get, METH_NOARGS, "Retrieves an item from this queue's head"},
  {"task_done", (PyCFunction)Queue_task_done, METH_NOARGS, "Indicates a task is done"},
  {"qsize", (PyCFunction)Queue_qsize, METH_NOARGS, "Returns this queue's size"},
  {NULL}
};


static PyTypeObject QueueType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "signal_queue.Queue",
  .tp_doc = "Signal handler safe queue",
  .tp_basicsize = sizeof(QueueObject),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_new = Queue_new,
  .tp_dealloc = (destructor) Queue_dealloc,
  .tp_methods = Queue_methods,
};


static struct PyModuleDef signal_queue_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "signal_queue",
    .m_doc = NULL, // no documentation
    .m_size = -1,
};


PyMODINIT_FUNC
PyInit_signal_queue() {
  if (PyType_Ready(&QueueType) < 0) {
    return nullptr;
  }

  PyObject* m = PyModule_Create(&signal_queue_module);
  if (m == nullptr) {
    return nullptr;
  }

  Py_INCREF(&QueueType);
  if (PyModule_AddObject(m, "Queue", (PyObject*) &QueueType) < 0) {
    Py_DECREF(&QueueType);
    Py_DECREF(m);
    return nullptr;
  }

  return m;
}
