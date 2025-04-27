#
# Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS),
# Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
# use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
# KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
# Mulan PSL v2 for more details.
#

import os
import ctypes
from typing import List


# find the shared library
current_path = os.path.dirname(os.path.abspath(__file__))
send_lib_path = os.path.join(current_path, "libsend.so")
llama_lib_path = os.path.join(
    current_path,
    "..",
    "build",
    "lib",
    "llama",
    "libsimple_llama_chat_shared.so",
)
send_lib = ctypes.CDLL(send_lib_path, mode=ctypes.RTLD_GLOBAL)
llama_lib = ctypes.CDLL(llama_lib_path, mode=ctypes.RTLD_GLOBAL)


####################### Below are C functions #######################


def ctypes_function(argtypes: List[any], restype: any):
    """
    A decorator for C functions
    """

    def decorator(func):
        func.argtypes = argtypes
        func.restype = restype
        return func

    return decorator


def initialize_llama_chat_wrapper(args: List[str]) -> int:
    """
    Python-friendly wrapper:
    takes a list of strings, transforms to (char**) for C.
    """
    argc = len(args)
    arr_type = ctypes.c_char_p * argc
    arr = arr_type(*(s.encode() for s in args))
    return initialize_llama_chat(argc, arr)


@ctypes_function(
    [ctypes.c_int, ctypes.POINTER(ctypes.POINTER(ctypes.c_char))], ctypes.c_int
)
def initialize_llama_chat(argc: int, argv) -> int:
    return llama_lib.initialize_llama_chat(argc, argv)


@ctypes_function([], None)
def free_llama_chat() -> None:
    llama_lib.free_llama_chat()


@ctypes_function([ctypes.c_char_p], ctypes.c_int)
def add_chat_user(user_name: bytes) -> int:
    return llama_lib.add_chat_user(user_name)


@ctypes_function([ctypes.c_char_p], ctypes.c_int)
def remove_chat_user(user_name: bytes) -> int:
    return llama_lib.remove_chat_user(user_name)


@ctypes_function([ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p], ctypes.c_int)
def quest_for_response(conn: int, client_name: bytes, current_input: bytes) -> int:
    return llama_lib.quest_for_response(conn, client_name, current_input)


####################### Above are C functions #######################
