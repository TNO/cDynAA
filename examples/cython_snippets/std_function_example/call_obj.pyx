from libcpp.string cimport string

cdef public void call_obj(obj, int a, const string& b):
    obj(a,b)
