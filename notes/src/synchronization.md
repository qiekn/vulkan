# Synchronization: Semaphores & Fences

Vulkan is radically asynchronous. The CPU records commands and submits them at blazing speed; the GPU executes them later, on its own timeline. Without explicit synchronization, the CPU would happily overwrite a command buffer the GPU is still reading, or the display engine would show a half-rendered frame.

There are exactly **two synchronization primitives** to learn:

| | Fence | Semaphore |
|---|---|---|
| Direction | GPU &rarr; CPU | GPU &rarr; GPU |
| CPU visible? | Yes (`waitForFences`) | No |
| Purpose | CPU waits for GPU to finish | Order GPU operations internally |

## Fence: The Buzzer

A fence is like a restaurant buzzer. The GPU buzzes it when it's done. The CPU blocks on `waitForFences()` until the buzz comes.

**Critical rule:** once a fence is signaled, the CPU *must* manually reset it via `resetFences()`. If you forget, the next `waitForFences()` returns instantly (it's already signaled!), and the CPU charges ahead into GPU memory that's still being used. Crash.

### Creation

Fences start **signaled** on purpose. On the very first frame, `DrawFrame()` calls `waitForFences()` before any submit has happened. If the fence started unsignaled, it would block forever.

```cpp
// one fence per frame-in-flight slot
for (auto _ : std::views::iota(0uz, kMAX_FRAMES_IN_FLIGHT)) {
    in_flight_fences_.emplace_back(
        device_,
        vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}
    );
}
```

### Usage in DrawFrame

```cpp
void DrawFrame() {
    // Block CPU until GPU finishes with this frame slot's command buffer
    auto fence_result = device_.waitForFences(
        *in_flight_fences_[frame_index_], vk::True, UINT64_MAX);

    // MUST reset immediately after wait succeeds
    device_.resetFences(*in_flight_fences_[frame_index_]);

    // ... acquire, record, ...

    // Submit passes the fence — GPU will signal it when done
    graphics_queue_.submit(submit_info, *in_flight_fences_[frame_index_]);

    // ...
}
```

The fence protects the **command buffer**. We have `kMAX_FRAMES_IN_FLIGHT = 2` frame slots, each with its own command buffer. The fence ensures that when slot 0 comes around again, the GPU has finished executing slot 0's previous commands before we overwrite them.


## Semaphore: The Traffic Light

Semaphores coordinate the ordering of GPU-side operations. The CPU can't see or wait on them — it just sets up the rules at submit time, and the GPU follows them internally.

We use two semaphores per frame cycle:

### present\_complete\_semaphore (acquire &rarr; render)

```cpp
// Acquire an image from the swapchain
auto [result, image_index] = swapchain_.acquireNextImage(
    UINT64_MAX,
    *present_complete_semaphores_[frame_index_],  // <-- GPU will signal this
    nullptr);
```

`acquireNextImage` returns an image index immediately, but the image may not be safe to write yet (the display engine might still be reading it). The semaphore is a promise: "when I'm signaled, the image is truly yours to render into."

Then at submit time, we tell the GPU to **wait** on this semaphore before it starts writing color data:

```cpp
vk::SubmitInfo submit_info{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &*present_complete_semaphores_[frame_index_],
    .pWaitDstStageMask = &wait_stage,  // eColorAttachmentOutput
    // ...
};
```

`pWaitDstStageMask = eColorAttachmentOutput` is subtle: the GPU can start *earlier* pipeline stages (vertex processing, etc.) before the semaphore fires. It only blocks at the exact stage where we'd write pixels. This maximizes parallelism.

### render\_finished\_semaphore (render &rarr; present)

```cpp
vk::SubmitInfo submit_info{
    // ...
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &*render_finished_semaphores_[image_index],
};
graphics_queue_.submit(submit_info, *in_flight_fences_[frame_index_]);
```

After the GPU finishes all commands in this submit, it signals `render_finished`. Then we tell `presentKHR` to wait on it:

```cpp
vk::PresentInfoKHR present_info{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &*render_finished_semaphores_[image_index],
    .swapchainCount = 1,
    .pSwapchains = &*swapchain_,
    .pImageIndices = &image_index,
};
graphics_queue_.presentKHR(present_info);
```

Without this, the display engine could push a half-drawn image to the screen.


## Why Different Counts?

This is the gotcha that confused me at first. The sync objects are **not** all the same count:

```cpp
// Indexed by frame_index_ (0..MAX_FRAMES_IN_FLIGHT-1)
std::vector<vk::raii::Semaphore> present_complete_semaphores_;  // size = 2
std::vector<vk::raii::Fence>     in_flight_fences_;             // size = 2
std::vector<vk::raii::CommandBuffer> command_buffers_;           // size = 2

// Indexed by image_index (0..swapchain_images_.size()-1)
std::vector<vk::raii::Semaphore> render_finished_semaphores_;   // size = 3 (usually)
```

**Frame-indexed (2):** These belong to the "frame slot" — the CPU rotates between slot 0 and slot 1. The fence ensures we don't reuse a slot that the GPU is still executing. The present\_complete semaphore is tied to the acquire call, which happens once per frame slot.

**Image-indexed (3):** The swapchain typically has 3 images. `render_finished` protects a specific *image* — "don't present image #2 until it's fully drawn." Different frame slots can end up drawing to the same image index, so we need one semaphore per image.

Creation code:

```cpp
void CreateSyncObjects() {
    // One render_finished per swapchain image
    for (auto _ : std::views::iota(0uz, swapchain_images_.size())) {
        render_finished_semaphores_.emplace_back(device_, vk::SemaphoreCreateInfo());
    }

    // One present_complete + one fence per frame-in-flight slot
    for (auto _ : std::views::iota(0uz, kMAX_FRAMES_IN_FLIGHT)) {
        present_complete_semaphores_.emplace_back(device_, vk::SemaphoreCreateInfo());
        in_flight_fences_.emplace_back(
            device_,
            vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}
```


## The &ast; Syntax

You'll see `&*` a lot in the submit/present calls. Here's what it does:

```cpp
.pWaitSemaphores = &*present_complete_semaphores_[frame_index_],
```

- `present_complete_semaphores_[frame_index_]` is a `vk::raii::Semaphore` (RAII wrapper)
- `*` calls `operator*()` which returns a `const vk::Semaphore&` (the raw Vulkan handle)
- `&` takes the address of that handle, giving `const vk::Semaphore*`

The Vulkan C API needs raw pointers. `&*` bridges the RAII wrapper to the C interface.


## Complete DrawFrame Walkthrough

Here's the full function with annotations showing which sync object does what:

```cpp
void DrawFrame() {
    // ---- STEP 1: Wait for this frame slot to be free ----
    // Fence was signaled by the PREVIOUS submit that used this slot.
    // First frame: fence starts signaled, so this returns immediately.
    device_.waitForFences(*in_flight_fences_[frame_index_], vk::True, UINT64_MAX);
    device_.resetFences(*in_flight_fences_[frame_index_]);

    // ---- STEP 2: Acquire a swapchain image ----
    // present_complete_semaphore will be signaled when the image
    // is actually available for rendering (display engine released it).
    auto [result, image_index] = swapchain_.acquireNextImage(
        UINT64_MAX, *present_complete_semaphores_[frame_index_], nullptr);

    // ---- STEP 3: Record commands ----
    // Safe to reset — fence confirmed GPU is done with this command buffer.
    command_buffers_[frame_index_].reset();
    RecordCommandBuffer(image_index);

    // ---- STEP 4: Submit to GPU ----
    vk::PipelineStageFlags wait_stage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*present_complete_semaphores_[frame_index_],
            // ^ GPU waits for image to be available before writing pixels
        .pWaitDstStageMask = &wait_stage,
            // ^ Only block at color output stage, vertex work can proceed early
        .commandBufferCount = 1,
        .pCommandBuffers = &*command_buffers_[frame_index_],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*render_finished_semaphores_[image_index],
            // ^ GPU signals this when all commands complete
    };
    graphics_queue_.submit(submit_info, *in_flight_fences_[frame_index_]);
        // ^ Fence signaled when GPU finishes — unblocks next waitForFences

    // ---- STEP 5: Present ----
    vk::PresentInfoKHR present_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*render_finished_semaphores_[image_index],
            // ^ Display engine waits until rendering is fully done
        .swapchainCount = 1,
        .pSwapchains = &*swapchain_,
        .pImageIndices = &image_index,
    };
    graphics_queue_.presentKHR(present_info);

    // ---- STEP 6: Advance frame slot ----
    frame_index_ = (frame_index_ + 1) % kMAX_FRAMES_IN_FLIGHT;
}
```


## Timeline: Two Frames In Flight

Here's what happens when `MAX_FRAMES_IN_FLIGHT = 2`:

```
Time ──────────────────────────────────────────────────────────────►

CPU:  [slot0: wait+acquire+record+submit] [slot1: wait+acquire+record+submit] [slot0: wait...
                                            │                                    │
GPU:  ·········[slot0: render──────────────]│[slot1: render──────────────]        │
                                         fence0                               fence1
                                        signaled                             signaled
```

- While GPU renders slot 0, CPU is already recording slot 1.
- When CPU circles back to slot 0, `waitForFences` blocks until GPU finishes slot 0.
- This keeps both CPU and GPU busy — neither waits for the other (except at the fence).

The gotcha: if you used `queue.waitIdle()` instead of fences, every frame would fully serialize:

```
CPU:  [record+submit] ......waiting...... [record+submit] ......waiting......
GPU:  ................[render]............[render]
```

Fences let us overlap CPU and GPU work — that's the whole point of frames-in-flight.


## Shutdown: waitIdle

Before destroying any Vulkan objects, we must ensure the GPU is completely idle:

```cpp
void MainLoop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        DrawFrame();
    }
    device_.waitIdle();  // block until ALL GPU work finishes
}
```

Without this, RAII destructors would destroy semaphores/fences/command buffers that the GPU is still using.
