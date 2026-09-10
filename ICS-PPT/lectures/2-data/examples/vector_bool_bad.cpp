/* This does not compile: vector<bool> does not store bool objects. */
#include <vector>

int main() {
    std::vector<bool> v(8);
    bool *p = &v[0];            /* v[0] is a proxy, not a bool */
    return *p;
}
