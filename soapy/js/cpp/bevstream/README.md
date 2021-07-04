# Turnstile
Modeled after `ByteTurnstileStream`.  This is missing the chunk argument / functionality because I think we dont need it

# CoarseSync
Modeled after `CoarseSync`.  Adds a debug option triggerCoarseAt which allows a coarse at specific lifetime frame, useful for test benches.  The results of coarse sync will not be relayed back to javascript until the next data block exits the stream (due to lazyness).

# FFT
Similar to `CF64StreamFFT` but scale is different.  Mode is always `ifft`.  This has `removeInputCp` `addOutputCp`.