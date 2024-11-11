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

#include <stdio.h>

namespace fooxx
{
template <typename T> class Foo {
    public:
        Foo(T &&val)
                : val(val)
        {
        }

        void inc()
        {
                val = val + static_cast<T>(1);
        }

        auto get_val() const -> T
        {
                return val;
        }

    private:
        T val;
};
}

extern "C" {
void foo(int val)
{
        auto foo = fooxx::Foo<int>(static_cast<int>(val));
        printf("before inc, val = %d\n", foo.get_val());
        foo.inc();
        printf("after inc, val = %d\n", foo.get_val());
}
}
