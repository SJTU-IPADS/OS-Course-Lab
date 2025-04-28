import os
import sys

# add ../build/ref/pyx to sys.path
sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), "../build/ref/pyx"))
)

import pyclient

if __name__ == "__main__":
    pyclient.client()
