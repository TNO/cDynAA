all:
	python setup.py build_ext --inplace

test: all
	python immutable_example.py

clean:
	@rm -f immutable_wrap.cpp *.o *.so *~ core
	@rm -rf build
