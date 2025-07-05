/*
 * MIT License
 * Copyright (c) 2022 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "lvgl/lvgl.h"
#include "vfs/vfs_implementation.h"
#include "lv_mem_macro.h"

libretro_vfs_implementation_file* retro_vfs_file_open_impl(const char* path, unsigned mode, unsigned hints)
{
    lv_fs_mode_t fs_mode = 0;
    if (mode & RETRO_VFS_FILE_ACCESS_READ) {
        fs_mode |= LV_FS_MODE_RD;
    }

    if (mode & RETRO_VFS_FILE_ACCESS_WRITE) {
        fs_mode |= LV_FS_MODE_WR;
    }

    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, path, fs_mode);

    if (res != LV_FS_RES_OK) {
        return NULL;
    }

    libretro_vfs_implementation_file* stream = lv_malloc(sizeof(libretro_vfs_implementation_file));
    LV_ASSERT_MALLOC(stream);
    lv_memset_00(stream, sizeof(libretro_vfs_implementation_file));

    lv_fs_file_t* file_p = lv_malloc(sizeof(lv_fs_file_t));
    LV_ASSERT_MALLOC(file_p);
    *file_p = file;

    stream->fp = (FILE*)file_p;

    return stream;
}

int retro_vfs_file_close_impl(libretro_vfs_implementation_file* stream)
{
    lv_fs_file_t* file_p = (lv_fs_file_t*)stream->fp;
    lv_fs_res_t res = lv_fs_close(file_p);
    lv_free(file_p);
    lv_free(stream);
    return res == LV_FS_RES_OK ? 0 : -1;
}

int retro_vfs_file_error_impl(libretro_vfs_implementation_file* stream)
{
    return -1;
}

int64_t retro_vfs_file_size_impl(libretro_vfs_implementation_file* stream)
{
    int64_t retval = -1;
    lv_fs_file_t* file_p = (lv_fs_file_t*)stream->fp;
    lv_fs_seek(file_p, 0, LV_FS_SEEK_END);

    uint32_t size;
    if (lv_fs_tell(file_p, &size) == LV_FS_RES_OK) {
        retval = size;
    }

    lv_fs_seek(file_p, 0, LV_FS_SEEK_SET);
    return retval;
}

int64_t retro_vfs_file_truncate_impl(libretro_vfs_implementation_file* stream, int64_t length)
{
    return -1;
}

int64_t retro_vfs_file_tell_impl(libretro_vfs_implementation_file* stream)
{
    int64_t retval = -1;
    lv_fs_file_t* file_p = (lv_fs_file_t*)stream->fp;

    uint32_t size;
    if (lv_fs_tell(file_p, &size) == LV_FS_RES_OK) {
        retval = size;
    }

    return retval;
}

int64_t retro_vfs_file_seek_impl(libretro_vfs_implementation_file* stream, int64_t offset, int seek_position)
{
    lv_fs_file_t* file_p = (lv_fs_file_t*)stream->fp;
    lv_fs_res_t res = lv_fs_seek(file_p, offset, seek_position);
    return res == LV_FS_RES_OK ? 0 : -1;
}

int64_t retro_vfs_file_read_impl(libretro_vfs_implementation_file* stream, void* s, uint64_t len)
{
    lv_fs_file_t* file_p = (lv_fs_file_t*)stream->fp;

    uint32_t br;
    lv_fs_res_t res = lv_fs_read(file_p, s, len, &br);
    return res == LV_FS_RES_OK ? (int64_t)br : -1;
}

int64_t retro_vfs_file_write_impl(libretro_vfs_implementation_file* stream, const void* s, uint64_t len)
{
    lv_fs_file_t* file_p = (lv_fs_file_t*)stream->fp;

    uint32_t bw;
    lv_fs_res_t res = lv_fs_write(file_p, s, len, &bw);
    return res == LV_FS_RES_OK ? (int64_t)bw : -1;
}

int retro_vfs_file_flush_impl(libretro_vfs_implementation_file* stream)
{
    return -1;
}

int retro_vfs_file_remove_impl(const char* path)
{
    return -1;
}

int retro_vfs_file_rename_impl(const char* old_path, const char* new_path)
{
    return -1;
}

const char* retro_vfs_file_get_path_impl(libretro_vfs_implementation_file* stream)
{
    return NULL;
}

int retro_vfs_stat_impl(const char* path, int32_t* size)
{
    return -1;
}

int retro_vfs_mkdir_impl(const char* dir)
{
    return -1;
}

libretro_vfs_implementation_dir* retro_vfs_opendir_impl(const char* dir, bool include_hidden)
{
    return NULL;
}

bool retro_vfs_readdir_impl(libretro_vfs_implementation_dir* dirstream)
{
    return false;
}

const char* retro_vfs_dirent_get_name_impl(libretro_vfs_implementation_dir* dirstream)
{
    return NULL;
}

bool retro_vfs_dirent_is_dir_impl(libretro_vfs_implementation_dir* dirstream)
{
    return false;
}

int retro_vfs_closedir_impl(libretro_vfs_implementation_dir* dirstream)
{
    return -1;
}
