#!/usr/bin/env python3
import ctypes
print("Content-Type: text/html\n")
ctypes.string_at(0)  # provoque un segfault
