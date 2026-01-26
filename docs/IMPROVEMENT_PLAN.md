# Spatial Audio System Improvement Plan

Based on the analysis of `SpatialAudioWorld`, the following improvements are proposed to enhance performance, realism, and robustness.

| Phase | Item | Description | Complexity | Impact |
| :--- | :--- | :--- | :--- | :--- |
| **1. Robustness** | **Voice Fading** | Implement short Gain Ramps (fade-in/out) during `ToMute` and `ToHear` to eliminate audio popping. | Low | Local (Audio Quality) |
| **2. Physics** | **Doppler Smoothing** | Apply low-pass filtering or interpolation to calculated velocity to prevent pitch jitter caused by frame rate fluctuations. | Low | Local (Realism) |
| **3. Scheduling** | **Priority System** | Add a `priority` field to `SpatialAudioSource`. Use `gain * priority` for voice stealing when physical sources are exhausted. | Medium | Global (Stability) |
| **4. Acoustics** | **Occlusion Support** | Integrate ray-casting hooks to apply low-pass filters when sources are blocked by geometry. | Medium | Global (Immersion) |
| **5. Performance** | **Spatial Indexing** | Implement an Octree or Grid index to optimize the `Update` loop from $O(N)$ to $O(\log N)$. | High | Global (Efficiency) |
| **6. Architecture** | **Reverb Blending** | Support multiple auxiliary slots and smooth transitions between different environmental reverb presets. | High | Global (System) |
