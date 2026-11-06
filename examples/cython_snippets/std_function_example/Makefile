all:
	python3 setup.py build_ext --inplace

test: all
	python3 callback_example.py

clean:
	@rm -f std_func_callback.cpp call_obj.h call_obj.cpp *.o *.so *~ core
	@rm -rf build
