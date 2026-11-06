import numpy as np
import std_func_callback as sfc

sfc.test_call()


def example4(a, b):
    """Pure python function that uses numpy"""
    # TODO would be interesting to compare speed with cython function using numpy
    # in complex way (i.e. lots of indexing etc.)
    print("{}!".format(b.decode().upper()))
    foo = np.ones((a, a))
    bar = np.mat("1 0; 0 1")
    print(np.kron(bar, foo))


# Send callback function to a c++ accepting std::function as argument:
sfc.call_func(example4)
