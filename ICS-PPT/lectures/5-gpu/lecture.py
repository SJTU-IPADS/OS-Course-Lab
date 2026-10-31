"""GPU 与 CUDA 编程。

四个部分：GPU 的结构，CUDA 编程，线程块与 Warp 的执行，框架中的算子调用。
结构在 CUDA 之前，行为在 CUDA 之后。前三部分的程序是向量内积 dot：第二部分写出，
第三部分分析它的执行。第四部分以 PyTorch 为例说明矩阵乘法怎样分派到 CUDA 核函数，并把自己写的核函数登记为算子。
以 lectures/ICS2/2-3-gpu（Mingcong Han 的 GPU 一讲）与 PyTorch 的源码和文档为素材。取舍记在 README.md。

页面函数在 pages.py 中，顺序与本文件中页面的顺序相同。函数名把下划线换成连字符就是页面的 id。
"""

from lecturekit.dsl import Lecture

import pages


lecture = Lecture(
    id="ics-ex-gpu",
    title="GPU 与 CUDA 编程",
    subtitle="从图形渲染到算子调用",
    ratio="16:9",
)

lecture.cover(
    "GPU 与 CUDA 编程",
    author="古金宇 · 陈榕",
    time="上海交通大学 IPADS",
)
lecture.page("outline", body=pages.outline)

lecture.bridge("第一部分：GPU 的结构", id="bridge-part1")

with lecture.section("第一部分：GPU 的结构", id="part1") as s:
    s.page("render-task", body=pages.render_task)
    s.page("render-realtime", body=pages.render_realtime)
    s.page("render-steps", body=pages.render_steps)
    s.page("gpu-cores", body=pages.gpu_cores)
    s.page("dgpu-igpu", body=pages.dgpu_igpu)
    s.page("gpu-sm", body=pages.gpu_sm)
    s.page("sm", body=pages.sm)
    s.page("simt", body=pages.simt)
    s.page("mem-hierarchy", body=pages.mem_hierarchy)
    s.page("flops-trend", body=pages.flops_trend)
    s.page("tensor-core", body=pages.tensor_core)
    s.page("exercise-structure", body=pages.exercise_structure)
    s.page("exercise-structure-2", body=pages.exercise_structure_2)

lecture.bridge("第二部分：CUDA 编程", id="bridge-part2")

with lecture.section("第二部分：CUDA 编程", id="part2") as s:
    s.page("cuda-model", body=pages.cuda_model)
    s.page("vecadd-kernel", body=pages.vecadd_kernel)
    s.page("thread-index", body=pages.thread_index)
    s.page("block-to-sm", body=pages.block_to_sm)
    s.page("vecadd-host", body=pages.vecadd_host)
    s.page("dot-kernel", body=pages.dot_kernel)
    s.page("dot-warp", body=pages.dot_warp)
    s.page("dot-race", body=pages.dot_race)
    s.page("dot-sync", body=pages.dot_sync)
    s.page("dot-fold", body=pages.dot_fold)
    s.page("dot-run", body=pages.dot_run)
    s.page("exercise-cuda", body=pages.exercise_cuda)
    s.page("exercise-cuda-2", body=pages.exercise_cuda_2)

lecture.bridge("第三部分：线程块与 Warp 的执行", id="bridge-part3")

with lecture.section("第三部分：线程块与 Warp 的执行", id="part3") as s:
    s.page("block-stages", body=pages.block_stages)
    s.page("warp-switch", body=pages.warp_switch)
    s.page("divergence", body=pages.divergence)
    s.page("diverge-run", body=pages.diverge_run)
    s.page("exercise-exec", body=pages.exercise_exec)
    s.page("exercise-exec-2", body=pages.exercise_exec_2)

lecture.bridge("第四部分：框架中的算子调用", id="bridge-part4")

with lecture.section("第四部分：框架中的算子调用", id="part4") as s:
    s.page("torch-matmul", body=pages.torch_matmul)
    s.page("torch-layers", body=pages.torch_layers)
    s.page("custom-op-flow", body=pages.custom_op_flow)
    s.page("custom-op-cuda", body=pages.custom_op_cuda)
    s.page("custom-op-register", body=pages.custom_op_register)
