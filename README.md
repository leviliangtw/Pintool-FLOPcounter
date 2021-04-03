# Pintool-FLOPcounter
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
    => Then modifiy the global variables "target_image" and "target_routines" in flop_counter.cpp
$ make
$ pin -t ./obj-intel64/flop_counter.so -- <Target_Program>
```

## Sample Result
`$ pin -t ./obj-intel64/flop_counter.so -- ./obj-intel64/matrix_multiplications.exe`
```
===============================================
This application is instrumented by MyPinTool
===============================================
[INFOS] Image Name: matrix_multiplications.exe, 0
        [INFOS] Decorated Routine Name: _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0.cold
        [INFOS] Decorated Routine Name: _Z20multiplySparseMatrixP9cs_sparseS0_S0_.cold
        [INFOS] Decorated Routine Name: _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
        [INFOS] Decorated Routine Name: _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_
        [INFOS] Decorated Routine Name: _Z20multiplySparseMatrixP9cs_sparseS0_S0_
[INFOS] Image Name: ld-linux-x86-64.so.2, -1
[INFOS] Image Name: [vdso], -18
* Starting tid 0
[INFOS] Image Name: libpthread.so.0, -1
[INFOS] Image Name: libstdc++.so.6, -1
[INFOS] Image Name: libgcc_s.so.1, -1
[INFOS] Image Name: libc.so.6, -1
[INFOS] Image Name: libm.so.6, -1
* Starting tid 1
* Starting tid 2
* Starting tid 3
* Starting tid 4
###############################################
Child
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       14664.9ms
multiplySparseMatrix: 26923.2ms
###############################################
###############################################
Child
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       411.034ms
multiplySparseMatrix: 249.863ms
###############################################
###############################################
Child
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       457.048ms
multiplySparseMatrix: 260.115ms
###############################################
* Stopping tid 4, code: 0
* Stopping tid 1, code: 0
* Stopping tid 3, code: 0
###############################################
Child
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       416.994ms
multiplySparseMatrix: 247.955ms
###############################################
* Stopping tid 2, code: 0
###############################################
Mother
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       2228.02ms
multiplySparseMatrix: 1397.85ms
###############################################
* Stopping tid 0, code: 0
===============================================
           The Total Analysis Result           
===============================================
Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
Image:               matrix_multiplications.exe
Address:             0x404370
Calls:                        6
Instructions counts:       6132
FLOP counts (TODO):           0
FLOP instructions: 
    [XED_IFORM]                   [XED_CAT]  [XED_EXT]  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]  [#opd]  [opd1(bit/type/elm)]  [opd2(bit/type/elm)]  [opd3(bit/type/elm)]
    ADDSD_XMMsd_MEMsd                   SSE       SSE2       102      0              1          0       2           64/DOUBLE/1           64/DOUBLE/1
    MULSD_XMMsd_MEMsd                   SSE       SSE2       102      0              1          0       2           64/DOUBLE/1           64/DOUBLE/1
    UCOMISD_XMMsd_XMMsd                 SSE       SSE2       216      0              1          0       3           64/DOUBLE/1           64/DOUBLE/1              32/INT/1

Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
Image:               matrix_multiplications.exe
Address:             0x403930
Calls:                        6
Instructions counts:      14454
FLOP counts (TODO):           0
FLOP instructions: 
    [XED_IFORM]                   [XED_CAT]  [XED_EXT]  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]  [#opd]  [opd1(bit/type/elm)]  [opd2(bit/type/elm)]  [opd3(bit/type/elm)]
    ADDSD_XMMsd_XMMsd                   SSE       SSE2      1296      0              1          0       2           64/DOUBLE/1           64/DOUBLE/1
    MULSD_XMMsd_MEMsd                   SSE       SSE2      1296      0              1          0       2           64/DOUBLE/1           64/DOUBLE/1

===============================================
      The Multi-Threading Analysis Result      
===============================================
Thread ID: 4
Routine counts: 2
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_MEMsd                  17      0              1          0
        MULSD_XMMsd_MEMsd                  17      0              1          0
        UCOMISD_XMMsd_XMMsd                36      0              1          0
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_XMMsd                 216      0              1          0
        MULSD_XMMsd_MEMsd                 216      0              1          0

Thread ID: 3
Routine counts: 2
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_MEMsd                  17      0              1          0
        MULSD_XMMsd_MEMsd                  17      0              1          0
        UCOMISD_XMMsd_XMMsd                36      0              1          0
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_XMMsd                 216      0              1          0
        MULSD_XMMsd_MEMsd                 216      0              1          0

Thread ID: 2
Routine counts: 2
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_MEMsd                  17      0              1          0
        MULSD_XMMsd_MEMsd                  17      0              1          0
        UCOMISD_XMMsd_XMMsd                36      0              1          0
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_XMMsd                 216      0              1          0
        MULSD_XMMsd_MEMsd                 216      0              1          0

Thread ID: 1
Routine counts: 2
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_MEMsd                  17      0              1          0
        MULSD_XMMsd_MEMsd                  17      0              1          0
        UCOMISD_XMMsd_XMMsd                36      0              1          0
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_XMMsd                 216      0              1          0
        MULSD_XMMsd_MEMsd                 216      0              1          0

Thread ID: 0
Routine counts: 4
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_MEMsd                  17      0              1          0
        MULSD_XMMsd_MEMsd                  17      0              1          0
        UCOMISD_XMMsd_XMMsd                36      0              1          0
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_XMMsd                 216      0              1          0
        MULSD_XMMsd_MEMsd                 216      0              1          0
    Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
    Image:               
    Instructions counts:       1022
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_MEMsd                  17      0              1          0
        MULSD_XMMsd_MEMsd                  17      0              1          0
        UCOMISD_XMMsd_XMMsd                36      0              1          0
    Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
    Image:               
    Instructions counts:       2409
    FLOP counts (TODO):           0
    FLOP instructions: 
        [XED_IFORM]                  [counts]  [FMA]  [Scalar_SIMD]  [Mask_OP]
        ADDSD_XMMsd_XMMsd                 216      0              1          0
        MULSD_XMMsd_MEMsd                 216      0              1          0
```

## TODO List
* [x] Multi-threading support
* [ ] AVX512 Masking computation and execution counter 
     * Find the value of Mask Register: `xed3_operand_get_mask(xed_decoded_inst_t *)` -> need testing
     * Compute the number of "1" in the Mask Register
     * Compute the execution times based on the "1s" and "is FMA or not"
* [ ] Total FLOP computations

## Reference
* [Pin - A Dynamic Binary Instrumentation Tool](https://software.intel.com/content/www/us/en/develop/articles/pin-a-dynamic-binary-instrumentation-tool.html)
* [Pin: Pin 3.18 User Guide](https://software.intel.com/sites/landingpage/pintool/docs/98332/Pin/html/index.html)
* [X86 Encoder Decoder: X86 Encoder Decoder User Guide](https://intelxed.github.io/ref-manual/index.html)
* [Calculating “FLOP” using Intel® Software Development Emulator (Intel® SDE)](https://software.intel.com/content/www/us/en/develop/articles/calculating-flop-using-intel-software-development-emulator-intel-sde.html)
* [Roofline Performance Model](https://crd.lbl.gov/departments/computer-science/PAR/research/roofline/)