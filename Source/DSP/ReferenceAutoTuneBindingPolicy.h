#pragma once

namespace OpenTune {

// A binding can be used only while both placements are live and overlap on
// the timeline. The arrangement model enforces these rules when creating a
// binding; this small pure predicate is also used at execution time so a
// stale UI snapshot cannot make AUTO Ref mutate content.
struct ReferenceAutoTuneBindingPolicy {
    static bool isUsable(bool targetFound,
                         bool targetRetired,
                         bool referenceFound,
                         bool referenceRetired,
                         bool hasPositiveOverlap) noexcept
    {
        return targetFound
            && !targetRetired
            && referenceFound
            && !referenceRetired
            && hasPositiveOverlap;
    }
};

} // namespace OpenTune
