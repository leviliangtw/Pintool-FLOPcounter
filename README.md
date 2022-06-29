# Pin FLOP Counter
* This is a [PIN](https://software.intel.com/content/www/us/en/develop/articles/pin-a-dynamic-binary-instrumentation-tool.html) tool. 
* Measures the **`floating point operations (FLOP)`** in the given program. 
* The computation is based on the methodology in this article: [Calculating “FLOP” using Intel® Software Development Emulator (Intel® SDE)](https://software.intel.com/content/www/us/en/develop/articles/calculating-flop-using-intel-software-development-emulator-intel-sde.html). 

## Content
* **`flop_counter.cpp`**: find the `target image` and instrument the `target routines` to record execution counts and necessary informations. 
* **`matrix_multiplications.cpp`**: a sample program implementing `normal matrix multiplications` and `sparse matrix multiplications`. 

## Build & Execute
```
$ cd ${pin_root}/source/tools/
$ git clone https://github.com/leviliangtw/Pintool-FLOPcounter.git
$ cd Pintool-FLOPcounter/
    => Then modifiy the global variables "target_routines" in flop_counter.cpp
$ make
$ pin -t ./obj-intel64/flop_counter.so -- <Target_Program>
```

## TODO List
* [X] Multi-threading support
* [X] AVX512 Masking computation and execution counter 
    * Find the value of Mask Register: `instruction_counter_mt()`
    * Compute the number of "1" in the Mask Register: `CountOnes()`
    * Compute the execution times based on the "1s" and "is FMA or not": `CalculateFLOP()`
* [X] Total FLOP computations: `CalculateFLOP()`
* [X] Optimization: 
    * Construct global `InsAttr` struct for the basic information of each instruction
    * Modify analysis routines so that they do not record overlapped or useless information
    * Remove the instrumentation to count the total number of instructions, instead calculate totals based on the statistics of each thread
    * Finally decrease the runtime of instrumented `polybench-c-3.2/2mm_time` from `1426.344537s` to `98.830370s`
* [ ] Bug: there is an inaccurate count of instructions when a switch between caller and callee (routines) is happened
* [ ] Test with AVX512 Masking instructions

## Sample Result
* [matrix_multiplications.exe](#matrix_multiplicationsexe)
* [polybench-c-3.2/atax_time](#polybench-c-32atax_time) (Cf. [Reference](#reference))
* [polybench-c-3.2/2mm_time](#polybench-c-322mm_time) (Cf. [Reference](#reference))

### matrix_multiplications.exe
```
$ pin -t ./obj-intel64/flop_counter.so -- ./obj-intel64/matrix_multiplications.exe
===============================================
This application is instrumented by MyPinTool
===============================================
[INFOS] Image Name: matrix_multiplications.exe, Target Name: matrix_multiplications.exe, 0
        [INFOS] Decorated Routine Name: _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0.cold
        [INFOS] Decorated Routine Name: _Z20multiplySparseMatrixP9cs_sparseS0_S0_.cold
        [INFOS] Decorated Routine Name: _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
        [INFOS] Decorated Routine Name: _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_
        [INFOS] Decorated Routine Name: _Z20multiplySparseMatrixP9cs_sparseS0_S0_
[INFOS] Image Name: ld-linux-x86-64.so.2, Target Name: matrix_multiplications.exe, -1
[INFOS] Image Name: [vdso], Target Name: matrix_multiplications.exe, -18
* Starting tid 0
[INFOS] Image Name: libpthread.so.0, Target Name: matrix_multiplications.exe, -1
[INFOS] Image Name: libstdc++.so.6, Target Name: matrix_multiplications.exe, -1
[INFOS] Image Name: libgcc_s.so.1, Target Name: matrix_multiplications.exe, -1
[INFOS] Image Name: libc.so.6, Target Name: matrix_multiplications.exe, -1
[INFOS] Image Name: libm.so.6, Target Name: matrix_multiplications.exe, -1
* Starting tid 1
* Starting tid 2
* Starting tid 3
* Starting tid 4
###############################################
Child
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       11629.1ms
multiplySparseMatrix: 14689.9ms
###############################################
###############################################
Child
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       114.918ms
multiplySparseMatrix: 102.997ms
###############################################
###############################################
Child
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       111.103ms
multiplySparseMatrix: 91.7912ms
###############################################
###############################################
Child
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       109.911ms
multiplySparseMatrix: 108.004ms
###############################################
###############################################
Mother
###############################################
A(6x6) multiply by B(6x6): 
* Stopping tid 2, code: 0
* Stopping tid 3, code: 0
* Stopping tid 1, code: 0
* Stopping tid 4, code: 0
multiplyMatrix:       17422.9ms
multiplySparseMatrix: 6668.09ms
multiplyMatrix:       16987.1ms
multiplySparseMatrix: 13021.9ms
###############################################
* Stopping tid 0, code: 0
===============================================
           The Total Analysis Result           
===============================================
Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
Image:               matrix_multiplications.exe
Address:             0x4044c0
Calls:                        6
Instructions counts:       6132
FLOP counts (TODO):         420
FLOP instructions: 
    [XED_IFORM]                   [XED_CAT]  [XED_EXT]    *[e_cnt]    *[c_cnt]  *[m_cnt]  *[FMA]  *[SS]  [MaskOP]  [#opd]        *[opd1]         [opd2]         [opd3]         [opd4]         [opd5]
    ADDSD_XMMsd_MEMsd                   SSE       SSE2         102         102         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    MULSD_XMMsd_MEMsd                   SSE       SSE2         102         102         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    UCOMISD_XMMsd_XMMsd                 SSE       SSE2         216         216         0       0      1         0       3    64/DOUBLE/1    64/DOUBLE/1       32/INT/1
    |-> MXCSR SIMD_SCALAR 
    * [e_cnt]: Executions Count. 
    * [c_cnt]: Computation Count. 
    * [m_cnt]: Masking Bits Count. 
    * [FMA]:   Fused Multiply-Add. 
    * [SS]:    Scalar SIMD. 
    * [opd]:   Shows bits, type and number of elements (#element). 

Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
Image:               matrix_multiplications.exe
Address:             0x403a80
Calls:                        6
Instructions counts:      14454
FLOP counts (TODO):        2592
FLOP instructions: 
    [XED_IFORM]                   [XED_CAT]  [XED_EXT]    *[e_cnt]    *[c_cnt]  *[m_cnt]  *[FMA]  *[SS]  [MaskOP]  [#opd]        *[opd1]         [opd2]         [opd3]         [opd4]         [opd5]
    ADDSD_XMMsd_XMMsd                   SSE       SSE2        1296        1296         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    MULSD_XMMsd_MEMsd                   SSE       SSE2        1296        1296         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    * [e_cnt]: Executions Count. 
    * [c_cnt]: Computation Count. 
    * [m_cnt]: Masking Bits Count. 
    * [FMA]:   Fused Multiply-Add. 
    * [SS]:    Scalar SIMD. 
    * [opd]:   Shows bits, type and number of elements (#element). 

===============================================
      The Multi-Threading Analysis Result      
===============================================
Thread ID: 4
Routine counts: 2
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):          70
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        MULSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        UCOMISD_XMMsd_XMMsd                  36          36        0      0     1         0                1
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):         432
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_XMMsd                   216         216        0      0     1         0                1
        MULSD_XMMsd_MEMsd                   216         216        0      0     1         0                1

Thread ID: 3
Routine counts: 2
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):          70
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        MULSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        UCOMISD_XMMsd_XMMsd                  36          36        0      0     1         0                1
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):         432
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_XMMsd                   216         216        0      0     1         0                1
        MULSD_XMMsd_MEMsd                   216         216        0      0     1         0                1

Thread ID: 2
Routine counts: 2
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):          70
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        MULSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        UCOMISD_XMMsd_XMMsd                  36          36        0      0     1         0                1
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):         432
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_XMMsd                   216         216        0      0     1         0                1
        MULSD_XMMsd_MEMsd                   216         216        0      0     1         0                1

Thread ID: 1
Routine counts: 2
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):          70
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        MULSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        UCOMISD_XMMsd_XMMsd                  36          36        0      0     1         0                1
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):         432
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_XMMsd                   216         216        0      0     1         0                1
        MULSD_XMMsd_MEMsd                   216         216        0      0     1         0                1

Thread ID: 0
Routine counts: 4
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):          70
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        MULSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        UCOMISD_XMMsd_XMMsd                  36          36        0      0     1         0                1
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):         432
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_XMMsd                   216         216        0      0     1         0                1
        MULSD_XMMsd_MEMsd                   216         216        0      0     1         0                1
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):          70
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        MULSD_XMMsd_MEMsd                    17          17        0      0     1         0                1
        UCOMISD_XMMsd_XMMsd                  36          36        0      0     1         0                1
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):         432
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_XMMsd                   216         216        0      0     1         0                1
        MULSD_XMMsd_MEMsd                   216         216        0      0     1         0                1
```

### polybench-c-3.2/atax_time
```
pin -t ./obj-intel64/flop_counter.so -- ./obj-intel64/polybench-c-3.2/atax_time
===============================================
This application is instrumented by MyPinTool
===============================================
[INFOS] Image Name: atax_time, Target Name: atax_time, 0
        [INFOS] Decorated Routine Name: main
[INFOS] Image Name: ld-linux-x86-64.so.2, Target Name: atax_time, 11
[INFOS] Image Name: [vdso], Target Name: atax_time, -6
* Starting tid 0
[INFOS] Image Name: libc.so.6, Target Name: atax_time, 11
0.889194
* Stopping tid 0, code: 0
===============================================
           The Total Analysis Result           
===============================================
Routine (Procedure): main
Image:               atax_time
Address:             0x55ede5e051c0
Calls:                        1
Instructions counts:  236160072
FLOP counts (TODO):    96020000
FLOP instructions: 
    [XED_IFORM]                   [XED_CAT]  [XED_EXT]    *[e_cnt]    *[c_cnt]  *[m_cnt]  *[FMA]  *[SS]  [MaskOP]  [#opd]        *[opd1]         [opd2]         [opd3]         [opd4]         [opd5]
    ADDPD_XMMpd_XMMpd                   SSE       SSE2     7996000    15992000         0       0      0         0       2    64/DOUBLE/2    64/DOUBLE/2
    |-> MXCSR REQUIRES_ALIGNMENT 
    ADDSD_XMMsd_MEMsd                   SSE       SSE2        8000        8000         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    ADDSD_XMMsd_XMMsd                   SSE       SSE2    16000000    16000000         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    DIVPD_XMMpd_XMMpd                   SSE       SSE2     8000000    16000000         0       0      0         0       2    64/DOUBLE/2    64/DOUBLE/2
    |-> MXCSR REQUIRES_ALIGNMENT 
    MULPD_XMMpd_XMMpd                   SSE       SSE2    15998000    31996000         0       0      0         0       2    64/DOUBLE/2    64/DOUBLE/2
    |-> MXCSR REQUIRES_ALIGNMENT 
    MULSD_XMMsd_MEMsd                   SSE       SSE2    16008000    16008000         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    UNPCKLPD_XMMpd_XMMq                 SSE       SSE2        8000       16000         0       0      0         0       2    64/DOUBLE/2       64/INT/1
    |-> REQUIRES_ALIGNMENT 
    * [e_cnt]: Executions Count. 
    * [c_cnt]: Computation Count. 
    * [m_cnt]: Masking Bits Count. 
    * [FMA]:   Fused Multiply-Add. 
    * [SS]:    Scalar SIMD. 
    * [opd]:   Shows bits, type and number of elements (#element). 

===============================================
      The Multi-Threading Analysis Result      
===============================================
Thread ID: 0
Routine counts: 1
    Routine (Procedure): main
    Image:               
    Instructions counts:  236160072
    FLOP counts (TODO):    96020000
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDPD_XMMpd_XMMpd               7996000    15992000        0      0     0         0                2
        ADDSD_XMMsd_MEMsd                  8000        8000        0      0     1         0                1
        ADDSD_XMMsd_XMMsd              16000000    16000000        0      0     1         0                1
        DIVPD_XMMpd_XMMpd               8000000    16000000        0      0     0         0                2
        MULPD_XMMpd_XMMpd              15998000    31996000        0      0     0         0                2
        MULSD_XMMsd_MEMsd              16008000    16008000        0      0     1         0                1
        UNPCKLPD_XMMpd_XMMq                8000       16000        0      0     0         0                2
```

### polybench-c-3.2/2mm_time
```
$ pin -t ./obj-intel64/flop_counter.so -- ./obj-intel64/polybench-c-3.2/2mm_time
===============================================
This application is instrumented by MyPinTool
===============================================
[INFOS] Image Name: 2mm_time, Target Name: 2mm_time, 0
        [INFOS] Decorated Routine Name: main
[INFOS] Image Name: ld-linux-x86-64.so.2, Target Name: 2mm_time, 58
[INFOS] Image Name: [vdso], Target Name: 2mm_time, 41
* Starting tid 0
[INFOS] Image Name: libc.so.6, Target Name: 2mm_time, 58
98.830370
* Stopping tid 0, code: 0
===============================================
           The Total Analysis Result           
===============================================
Routine (Procedure): main
Image:               2mm_time
Address:             0x55d15a5051a0
Calls:                        1
Instructions counts: 18289053784
FLOP counts (TODO):  5378154496
FLOP instructions: 
    [XED_IFORM]                   [XED_CAT]  [XED_EXT]    *[e_cnt]    *[c_cnt]  *[m_cnt]  *[FMA]  *[SS]  [MaskOP]  [#opd]        *[opd1]         [opd2]         [opd3]         [opd4]         [opd5]
    ADDSD_XMMsd_XMMsd                   SSE       SSE2  2147483648  2147483648         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    MULPD_XMMpd_XMMpd                   SSE       SSE2     4194304     8388608         0       0      0         0       2    64/DOUBLE/2    64/DOUBLE/2
    |-> MXCSR REQUIRES_ALIGNMENT 
    MULSD_XMMsd_MEMsd                   SSE       SSE2  2147483648  2147483648         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    MULSD_XMMsd_XMMsd                   SSE       SSE2  1074790400  1074790400         0       0      1         0       2    64/DOUBLE/1    64/DOUBLE/1
    |-> MXCSR SIMD_SCALAR 
    UNPCKLPD_XMMpd_XMMq                 SSE       SSE2        4096        8192         0       0      0         0       2    64/DOUBLE/2       64/INT/1
    |-> REQUIRES_ALIGNMENT 
    * [e_cnt]: Executions Count. 
    * [c_cnt]: Computation Count. 
    * [m_cnt]: Masking Bits Count. 
    * [FMA]:   Fused Multiply-Add. 
    * [SS]:    Scalar SIMD. 
    * [opd]:   Shows bits, type and number of elements (#element). 

===============================================
      The Multi-Threading Analysis Result      
===============================================
Thread ID: 0
Routine counts: 1
    Routine (Procedure): main
    Image:               
    Instructions counts: 18289053784
    FLOP counts (TODO):  5378154496
    FLOP instructions: 
        [XED_IFORM]                     [e_cnt]     [c_cnt]  [m_cnt]  [FMA]  [SS]  [MaskOP]  [#element_opd1]
        ADDSD_XMMsd_XMMsd            2147483648  2147483648        0      0     1         0                1
        MULPD_XMMpd_XMMpd               4194304     8388608        0      0     0         0                2
        MULSD_XMMsd_MEMsd            2147483648  2147483648        0      0     1         0                1
        MULSD_XMMsd_XMMsd            1074790400  1074790400        0      0     1         0                1
        UNPCKLPD_XMMpd_XMMq                4096        8192        0      0     0         0                2
```

## Reference
* [Pin - A Dynamic Binary Instrumentation Tool](https://software.intel.com/content/www/us/en/develop/articles/pin-a-dynamic-binary-instrumentation-tool.html)
* [Pin: Pin 3.18 User Guide](https://software.intel.com/sites/landingpage/pintool/docs/98332/Pin/html/index.html)
* [X86 Encoder Decoder: X86 Encoder Decoder User Guide](https://intelxed.github.io/ref-manual/index.html)
* [Calculating “FLOP” using Intel® Software Development Emulator (Intel® SDE)](https://software.intel.com/content/www/us/en/develop/articles/calculating-flop-using-intel-software-development-emulator-intel-sde.html)
* [Roofline Performance Model](https://crd.lbl.gov/departments/computer-science/PAR/research/roofline/)
* [PolyBench/C -- Homepage of Louis-Noël Pouchet](https://web.cse.ohio-state.edu/~pouchet.2/software/polybench/)
