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

import json


class MessageModel:
    """
    MessageModel is a class that represents a message with a token and
    an end-of-generation (EOG) flag. It provides methods to initialize the
    message, convert it to a string, and create an instance from a JSON string.
    """

    # Only two attributes
    token: str
    eog: bool

    def __init__(self, token: str = "", eog: bool = False, json_str: str = None):
        # parse the JSON string if provided
        if json_str is not None:
            data = json.loads(json_str)
            self.token = data.get("token", "")
            self.eog = data.get("eog", False)
        # initialize the attributes
        else:
            self.token = token
            self.eog = eog

    def __str__(self):
        return json.dumps({"token": self.token, "eog": self.eog})
