# About Renergy SDK

This SDK library code is suitable for 32-bit RISC-V dual-core MCUs,
of which Core1 is the application core, the performance is equivalent to M33,
and the highest frequency is 58.9824MHz. Core0 is a security core with
the same performance as M23 and meets the TEE architecture.
The highest frequency is 29.4912MHz.

It is recommended to use Core0 for single-core applications and
add macro`-DCORE0 -DUSE_FULL_LL_DIRVER` to the project.
If you need a higher frequency and need the FPU function,
you can choose to use Core1 and specify the macro `-DCORE1 -DUSE_FULL_LL_DIRVER`.
