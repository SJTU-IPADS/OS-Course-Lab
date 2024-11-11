add_library(vxworks STATIC msgq.c sem.c task.c intlock.c chcore-sysLib.c)
chcore_target_include_and_export_directory(vxworks PUBLIC ".")
