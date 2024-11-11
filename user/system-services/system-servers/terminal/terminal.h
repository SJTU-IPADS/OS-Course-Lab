/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#pragma once

#include <stdnoreturn.h>
#include <chcore/type.h>

#include "libtsm.h"

/**
 * \class terminal
 *
 * A terminal is a terminal application.
 */
struct terminal;

/**
 * Send to shell function type
 *
 * Functions of this type are used by terminals to send data to shells.
 *
 * \param buffer The data to send to shell
 * \param size The size of \p buffer
 */
typedef void (*send_to_shell_func_t)(const char buffer[], size_t size);

/**
 * Create a terminal.
 *
 * \param col_num The column number of the new terminal
 * \param row_num The row number of the new terminal
 * \param send_to_shell The send to shell function of the new terminal
 * \return The new terminal
 */
struct terminal *terminal_create(u32 col_num, u32 row_num,
                                 send_to_shell_func_t send_to_shell);

/**
 * Run \p terminal. At the end of the function is the Wayland main loop, so the
 * function does not return.
 *
 * \param terminal The terminal
 */
noreturn void terminal_run(struct terminal *terminal);

/**
 * The shell sends data \p terminal.
 *
 * \param terminal The terminal
 * \param buffer The data sended by the shell
 * \param size The size of \p buffer
 */
void terminal_put(struct terminal *terminal, const char buffer[], size_t size);
