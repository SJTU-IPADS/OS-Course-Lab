#!/usr/bin/env python3
"""Write four tiny files, one per way a model format identifies itself.

The lecture hexdumps the first bytes of each. Every file here is produced by
the format's own rules, not hand-copied bytes:

  tiny.safetensors  8-byte little-endian header length, then a JSON header
  tiny.onnx         a real ModelProto, serialised by the onnx package
  tiny.pkl          a pickle stream, the layout torch.save used before 1.6
  tiny.zip          a zip archive, the container torch.save uses since 1.6
"""

import json
import pathlib
import pickle
import struct
import zipfile

HERE = pathlib.Path(__file__).parent


def safetensors():
    """One 2x2 F32 tensor. Spec: u64 header length, JSON header, then data."""
    data = struct.pack("<4f", 0.0, 1.0, 2.0, 3.0)
    header = {"weight": {"dtype": "F32", "shape": [2, 2],
                         "data_offsets": [0, len(data)]}}
    blob = json.dumps(header, separators=(",", ":")).encode("utf-8")
    return struct.pack("<Q", len(blob)) + blob + data


def onnx_model():
    import numpy as np
    from onnx import TensorProto, helper, numpy_helper

    weight = numpy_helper.from_array(np.zeros((2, 2), dtype=np.float32), "weight")
    node = helper.make_node("Identity", ["weight"], ["out"])
    graph = helper.make_graph([node], "tiny", [], [
        helper.make_tensor_value_info("out", TensorProto.FLOAT, [2, 2])
    ], initializer=[weight])
    model = helper.make_model(graph, producer_name="lecturekit")
    model.ir_version = 8
    return model.SerializeToString()


def legacy_pickle():
    """What torch.save wrote before 1.6: a pickle stream, protocol 2."""
    return pickle.dumps({"weight": [0.0, 1.0, 2.0, 3.0]}, protocol=2)


def main():
    (HERE / "tiny.safetensors").write_bytes(safetensors())
    (HERE / "tiny.onnx").write_bytes(onnx_model())
    (HERE / "tiny.pkl").write_bytes(legacy_pickle())
    with zipfile.ZipFile(HERE / "tiny.zip", "w") as z:
        # A fixed timestamp, so regenerating the file reproduces the same bytes.
        entry = zipfile.ZipInfo("archive/data.pkl", date_time=(2024, 1, 1, 0, 0, 0))
        z.writestr(entry, legacy_pickle())


if __name__ == "__main__":
    main()
