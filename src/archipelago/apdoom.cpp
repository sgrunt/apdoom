#include <vector>

static std::vector<int> bar;

size_t foo()
{
    bar.push_back(42);
    return bar.size();
}
