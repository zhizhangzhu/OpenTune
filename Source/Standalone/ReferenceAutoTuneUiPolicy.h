#pragma once

namespace OpenTune {

// UI policy for the first-version reference-vocal workflow. The regular AUTO
// path remains the fallback whenever the selection has no usable reference or
// when the Notes-primary editing scheme owns the AUTO button.
struct ReferenceAutoTuneUiPolicy {
    static bool shouldUseReferenceAuto(bool usesNotesPrimaryScheme,
                                       bool hasReadyReferenceBinding) noexcept
    {
        return !usesNotesPrimaryScheme && hasReadyReferenceBinding;
    }
};

} // namespace OpenTune
