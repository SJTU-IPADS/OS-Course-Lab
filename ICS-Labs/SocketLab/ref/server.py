import os
import sys

# add ../build/ref/pyx to sys.path
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), "../build/ref/pyx"))
)

import pyserver

if __name__ == "__main__":
    pyserver.server()
