from distutils.core import setup, Extension

setup(name = 'SignalQueuePackage',
      version = '1.0',
      description = 'Signal handler safe queue',
      ext_modules = [
          Extension('signal_queue', sources = ['pysignal_queue.cxx'],
                    extra_compile_args=['-std=c++17'])
      ])
