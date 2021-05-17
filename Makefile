CXX=clang++
CXXFLAGS=-std=c++17 -O3 -g -DNDEBUG -fno-builtin-malloc -fPIC

run: build
	PYTHONPATH=$$PYTHONPATH:$(wildcard build/lib.*) python tryit.py

build: pysignal_queue.cxx setup.py
	python setup.py build

clean:
	-rm -rf build
