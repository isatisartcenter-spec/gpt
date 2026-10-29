#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <vector>

/** Single-producer audio ring with atomic samples for UI / OpenGL visualization. */
class VisualFifo
{
public:
    void prepare (int capacity)
    {
        auto next = std::make_unique<State> (juce::jmax (1024, capacity));
        for (int i = 0; i < next->size; ++i)
            next->samples[(size_t) i].store (0.0f, std::memory_order_relaxed);
        auto* published = next.get();
        // Keep old states alive: a UI read or an in-flight audio callback may
        // still hold the previous pointer during a host re-prepare.
        states.push_back (std::move (next));
        current.store (published, std::memory_order_release);
    }

    void push (const float* samples, int num)
    {
        auto* state = current.load (std::memory_order_acquire);
        if (state == nullptr || samples == nullptr || num <= 0) return;
        int w = state->writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < num; ++i)
        {
            state->samples[(size_t) w].store (samples[i], std::memory_order_relaxed);
            w = (w + 1) % state->size;
        }
        state->writePos.store (w, std::memory_order_release);
    }

    void pushStereo (const juce::AudioBuffer<float>& buf)
    {
        const int n = buf.getNumSamples();
        auto* state = current.load (std::memory_order_acquire);
        if (n <= 0 || state == nullptr || buf.getNumChannels() <= 0) return;
        const float* L = buf.getReadPointer (0);
        const float* R = buf.getNumChannels() > 1 ? buf.getReadPointer (1) : L;
        int w = state->writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            state->samples[(size_t) w].store (0.5f * (L[i] + R[i]), std::memory_order_relaxed);
            w = (w + 1) % state->size;
        }
        state->writePos.store (w, std::memory_order_release);
    }

    /** Copy latest `count` samples into dest (oldest to newest). */
    int readLatest (float* dest, int count) const
    {
        auto* state = current.load (std::memory_order_acquire);
        if (state == nullptr || dest == nullptr || count <= 0) return 0;
        count = juce::jmin (count, state->size);
        const int w = state->writePos.load (std::memory_order_acquire);
        int start = w - count;
        if (start < 0) start += state->size;
        for (int i = 0; i < count; ++i)
            dest[i] = state->samples[(size_t) ((start + i) % state->size)].load (std::memory_order_relaxed);
        return count;
    }

    int getCapacity() const noexcept
    {
        auto* state = current.load (std::memory_order_acquire);
        return state != nullptr ? state->size : 0;
    }

private:
    struct State
    {
        explicit State (int capacity)
            : size (capacity), samples (std::make_unique<std::atomic<float>[]> ((size_t) capacity)) {}
        const int size;
        std::unique_ptr<std::atomic<float>[]> samples;
        std::atomic<int> writePos { 0 };
    };

    std::vector<std::unique_ptr<State>> states;
    std::atomic<State*> current { nullptr };
};
