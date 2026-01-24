#pragma once

#include <atomic>
#include <juce_core/juce_core.h>

/**
 * A simple thread-safe circular buffer for visualization.
 * Audio thread pushes samples, GUI thread reads them for rendering.
 */
class VisualBuffer {
public:
  static constexpr int DEFAULT_SIZE = 1024;

  VisualBuffer(int size = DEFAULT_SIZE) : bufferSize(size), buffer(size, 0.0f) {
    writePos.store(0);
  }

  /** Pushes a single sample into the circular buffer. */
  void pushSample(float sample) {
    int pos = writePos.load();
    buffer[pos] = sample;
    writePos.store((pos + 1) % bufferSize);
  }

  /** Copies the current buffer state into a destination buffer for rendering.
   */
  void copyTo(std::vector<float> &dest) const {
    int pos = writePos.load();
    int size = std::min((int)dest.size(), bufferSize);

    for (int i = 0; i < size; ++i) {
      // Read starting from current write position (oldest sample)
      int readIdx = (pos + i) % bufferSize;
      dest[i] = buffer[readIdx];
    }
  }

  int getSize() const { return bufferSize; }

private:
  int bufferSize;
  std::vector<float> buffer;
  std::atomic<int> writePos;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualBuffer)
};
