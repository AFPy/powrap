import ctypes
lib = ctypes.cdll.LoadLibrary("./wrap.so")
lib.wrap.restype = ctypes.c_char_p
lib.wrap.argtypes = [ctypes.c_char_p]
def wrap(s):
    return lib.wrap(s.encode("UTF-8")).decode("UTF-8")
