#include "../Source/DSP/ReferenceAutoTuneBindingPolicy.h"

#include <cstdlib>
#include <iostream>

using namespace OpenTune;

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void testOnlyLiveOverlappingBindingIsUsable()
{
    expect(ReferenceAutoTuneBindingPolicy::isUsable(
               true, false, true, false, true),
           "live target and reference with overlap should be usable");
    expect(!ReferenceAutoTuneBindingPolicy::isUsable(
               false, false, true, false, true),
           "missing target should invalidate the binding");
    expect(!ReferenceAutoTuneBindingPolicy::isUsable(
               true, true, true, false, true),
           "retired target should invalidate the binding");
    expect(!ReferenceAutoTuneBindingPolicy::isUsable(
               true, false, true, true, true),
           "retired reference should invalidate the binding");
    expect(!ReferenceAutoTuneBindingPolicy::isUsable(
               true, false, true, false, false),
           "non-overlapping clips should invalidate the binding");
}

} // namespace

int main()
{
    testOnlyLiveOverlappingBindingIsUsable();
    if (failures != 0) {
        std::cerr << failures << " reference binding policy test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "ReferenceAutoTuneBindingPolicyTests passed\n";
    return EXIT_SUCCESS;
}
