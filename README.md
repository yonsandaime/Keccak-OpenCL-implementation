# Keccak-OpenCL-implementation

This is a SHA3-256 f[1600] implementation with with 24 rounds

There are multiple modes:
- Normal (1 thread)
- Parallel 5 theads: 5 threads are used for completing the state array
- Parallel 25 theads: 25 threads are used for completing the state array

- Batch mode: Allows the hashing of multiple files

- Buffering 136: 136 bytes are sent to the device per kernel
- Buffering 136N: 136*N bytes are sent to the device per kernel. N = 400, can be changed in the code.

- Dual buffering: In this implementation we use 2 streams. One is used to copy the input (136*N) to the device while the second stream is used for executing the kernel function. 1/5/25 thread variants are also available for this mode.



