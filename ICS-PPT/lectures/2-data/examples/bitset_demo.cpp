/* How C++ stores a million boolean values, two ways. */
#include <bitset>
#include <cstdio>
#include <deque>
#include <vector>

int main() {
    std::vector<bool> v(1000000);
    std::bitset<1000000> b;
    std::deque<bool> d(1000000);

    printf("1000000 bools  vector<bool> %zu  bitset %zu  deque %zu  (bytes)\n",
           v.capacity() / 8, sizeof(b), d.size() * sizeof(bool));
    return 0;
}
