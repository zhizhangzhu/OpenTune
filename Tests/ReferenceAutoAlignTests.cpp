#include "../Source/DSP/ReferenceAutoAlign.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

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

bool closeTo(double actual, double expected, double epsilon = 1e-6)
{
    return std::abs(actual - expected) <= epsilon;
}

ContentKey makeKey(uint64_t objectId)
{
    ContentKey key;
    key.domainKind = DomainKind::StandaloneClip;
    key.objectId = objectId;
    return key;
}

Note makeNote(double start, double end, float pitch)
{
    Note note;
    note.startTime = start;
    note.endTime = end;
    note.pitch = pitch;
    note.originalPitch = pitch;
    return note;
}

ReferenceFeatureSet readyFeatures(ReferenceFeatureProducer producer,
                                  std::vector<Note> notes)
{
    ReferenceFeatureSet features;
    features.status = ReferenceFeatureStatus::Ready;
    features.producer = producer;
    features.inputFingerprint = 1;
    features.sourceDurationSeconds = 10.0;
    features.pitch.notes = std::move(notes);
    return features;
}

ReferenceAlignmentRequest baseRequest()
{
    ReferenceAlignmentRequest request;
    request.target.placementId = 10;
    request.target.contentKey = makeKey(1);
    request.target.timelineStartSeconds = 0.0;
    request.target.timelineEndSeconds = 2.0;
    request.target.sourceStartSeconds = 0.0;
    request.target.sourceEndSeconds = 2.0;
    request.reference.placementId = 20;
    request.reference.contentKey = makeKey(2);
    request.reference.timelineStartSeconds = 0.0;
    request.reference.timelineEndSeconds = 2.0;
    request.reference.sourceStartSeconds = 0.0;
    request.reference.sourceEndSeconds = 2.0;
    request.targetTimeMap = EffectiveTimeMap::identity(10.0);
    request.referenceTimeMap = EffectiveTimeMap::identity(10.0);
    request.overlapStartTimelineSeconds = 0.0;
    request.overlapEndTimelineSeconds = 2.0;
    return request;
}

void testSameTimelineAlignsOnlyTargetNotes()
{
    auto request = baseRequest();
    request.targetNotesBefore = {makeNote(0.25, 1.25, 220.0f)};
    request.targetFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(0.25, 1.25, 440.0f)});
    request.referenceFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(0.25, 1.25, 440.0f)});

    const auto patch = ReferenceAutoAlign::align(request);
    expect(patch.success, "same-timeline reference should produce a patch");
    expect(patch.pitchChanged, "successful reference alignment should mark pitchChanged");
    expect(patch.notesAfter.size() == 1, "alignment should preserve target note topology");
    expect(closeTo(patch.notesAfter.front().pitch, 440.0),
           "alignment should copy the reference pitch to the target note");
    expect(patch.notesAfter.front().pitchOffset == 0.0f,
           "alignment should clear a previous target pitch offset");
    expect(patch.notesAfter.front().dirty,
           "alignment should mark the changed target note dirty");
}

void testDifferentClipInWindowsProjectToTheSameTimeline()
{
    auto request = baseRequest();
    request.target.timelineStartSeconds = 10.0;
    request.target.timelineEndSeconds = 12.0;
    request.target.sourceStartSeconds = 2.0;
    request.target.sourceEndSeconds = 4.0;
    request.reference.timelineStartSeconds = 10.0;
    request.reference.timelineEndSeconds = 12.0;
    request.reference.sourceStartSeconds = 5.0;
    request.reference.sourceEndSeconds = 7.0;
    request.overlapStartTimelineSeconds = 10.0;
    request.overlapEndTimelineSeconds = 12.0;
    request.targetNotesBefore = {makeNote(2.25, 3.25, 220.0f)};
    request.targetFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(2.25, 3.25, 220.0f)});
    request.referenceFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(5.25, 6.25, 440.0f)});

    const auto patch = ReferenceAutoAlign::align(request);
    expect(patch.success, "clip-in offsets should still align overlapping notes");
    expect(closeTo(patch.notesAfter.front().pitch, 440.0),
           "clip-in offsets should map the reference note onto the target note");
    expect(closeTo(patch.affectedSourceStartSeconds, 2.0),
           "affected source range should begin at the target clip-in");
    expect(closeTo(patch.affectedSourceEndSeconds, 4.0),
           "affected source range should end at the target clip-out");
}

void testNoOverlapDoesNotMutate()
{
    auto request = baseRequest();
    request.target.timelineStartSeconds = 0.0;
    request.target.timelineEndSeconds = 1.0;
    request.reference.timelineStartSeconds = 2.0;
    request.reference.timelineEndSeconds = 3.0;
    request.overlapStartTimelineSeconds = 2.0;
    request.overlapEndTimelineSeconds = 1.0;
    request.targetNotesBefore = {makeNote(0.0, 1.0, 220.0f)};
    request.targetFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(0.0, 1.0, 220.0f)});
    request.referenceFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(0.0, 1.0, 440.0f)});

    const auto patch = ReferenceAutoAlign::align(request);
    expect(!patch.success, "non-overlapping clips must not align");
    expect(patch.error == AlignmentPatch::ErrorCode::NoOverlap,
           "non-overlapping clips should report NoOverlap");
    expect(patch.notesAfter.empty(), "a rejected overlap should not return a mutation");
}

void testReferenceAnalysisMustBeReady()
{
    auto request = baseRequest();
    request.targetNotesBefore = {makeNote(0.0, 1.0, 220.0f)};
    request.targetFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(0.0, 1.0, 220.0f)});
    request.referenceFeatures.status = ReferenceFeatureStatus::Extracting;
    request.referenceFeatures.producer = ReferenceFeatureProducer::StandardAuto;

    const auto patch = ReferenceAutoAlign::align(request);
    expect(!patch.success, "pending reference analysis must not align");
    expect(patch.error == AlignmentPatch::ErrorCode::ReferenceAnalysisNotReady,
           "pending reference analysis should report ReferenceAnalysisNotReady");
}

void testEmptyReferenceFeaturesDoNotMutate()
{
    auto request = baseRequest();
    request.targetNotesBefore = {makeNote(0.0, 1.0, 220.0f)};
    request.targetFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(0.0, 1.0, 220.0f)});
    request.referenceFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto, {});

    const auto patch = ReferenceAutoAlign::align(request);
    expect(!patch.success, "empty reference features must not align");
    expect(patch.error == AlignmentPatch::ErrorCode::InsufficientFeatures,
           "empty reference features should report InsufficientFeatures");
}

void testEqualPitchIsNoMutation()
{
    auto request = baseRequest();
    request.targetNotesBefore = {makeNote(0.0, 1.0, 440.0f)};
    request.targetFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(0.0, 1.0, 440.0f)});
    request.referenceFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(0.0, 1.0, 440.0f)});

    const auto patch = ReferenceAutoAlign::align(request);
    expect(!patch.success, "equal pitches should not create a mutation");
    expect(patch.error == AlignmentPatch::ErrorCode::NoMutation,
           "equal pitches should report NoMutation");
    expect(!patch.pitchChanged, "equal pitches should not set pitchChanged");
}

void testUnknownReadyProducerIsRejected()
{
    auto request = baseRequest();
    request.targetNotesBefore = {makeNote(0.0, 1.0, 220.0f)};
    request.targetFeatures = readyFeatures(
        ReferenceFeatureProducer::StandardAuto,
        {makeNote(0.0, 1.0, 220.0f)});
    request.referenceFeatures = readyFeatures(
        ReferenceFeatureProducer::Unknown,
        {makeNote(0.0, 1.0, 440.0f)});

    const auto patch = ReferenceAutoAlign::align(request);
    expect(!patch.success, "unknown reference producer must not be trusted");
    expect(patch.error == AlignmentPatch::ErrorCode::InsufficientFeatures,
           "unknown reference producer should report InsufficientFeatures");
}

} // namespace

int main()
{
    testSameTimelineAlignsOnlyTargetNotes();
    testDifferentClipInWindowsProjectToTheSameTimeline();
    testNoOverlapDoesNotMutate();
    testReferenceAnalysisMustBeReady();
    testEmptyReferenceFeaturesDoNotMutate();
    testEqualPitchIsNoMutation();
    testUnknownReadyProducerIsRejected();

    if (failures != 0) {
        std::cerr << failures << " reference alignment test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "ReferenceAutoAlignTests passed\n";
    return EXIT_SUCCESS;
}
