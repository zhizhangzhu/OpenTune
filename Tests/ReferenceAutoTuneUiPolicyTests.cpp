#include "../Source/Standalone/ReferenceAutoTuneUiPolicy.h"

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

void testReferenceAutoWinsOnlyForNonNotesPrimaryWithBinding()
{
    expect(ReferenceAutoTuneUiPolicy::shouldUseReferenceAuto(false, true),
           "a valid reference binding should select AUTO Ref in standard mode");
    expect(!ReferenceAutoTuneUiPolicy::shouldUseReferenceAuto(false, false),
           "without a reference binding the regular AUTO path should remain selected");
    expect(!ReferenceAutoTuneUiPolicy::shouldUseReferenceAuto(true, true),
           "Notes-primary mode should keep its regular AUTO path");
}

} // namespace

int main()
{
    testReferenceAutoWinsOnlyForNonNotesPrimaryWithBinding();
    if (failures != 0) {
        std::cerr << failures << " reference UI policy test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "ReferenceAutoTuneUiPolicyTests passed\n";
    return EXIT_SUCCESS;
}
