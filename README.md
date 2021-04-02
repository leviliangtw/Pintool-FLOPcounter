# Pintool-FLOPcounter
* This is a [Pin](https://software.intel.com/content/www/us/en/develop/articles/pin-a-dynamic-binary-instrumentation-tool.html) tool. 
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
[INFOS] Image Name: libstdc++.so.6, -1
[INFOS] Image Name: libgcc_s.so.1, -1
[INFOS] Image Name: libc.so.6, -1
[INFOS] Image Name: libm.so.6, -1
###############################################
A(6x6) multiply by B(6x6): 
multiplyMatrix:       4611.02ms
multiplySparseMatrix: 8669.85ms
###############################################
===============================================
              The Analysis Result              
===============================================
Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_
Image:               matrix_multiplications.exe
Address:             0x403f30
Calls:                        1
Instructions counts:       1022
FLOP counts (TODO):           0
FLOP instructions:   
     XED_IFORM                XED_ICLASS   XED_CAT   XED_EXT    is_FMA    counts      #opd     opd_1/type       #elements     opd_2/type       #elements     opd_3/type       #elements
     ADDSD_XMMsd_MEMsd             ADDSD       SSE      SSE2         0        17         2        64/DOUBLE             1        64/DOUBLE             1
     MULSD_XMMsd_MEMsd             MULSD       SSE      SSE2         0        17         2        64/DOUBLE             1        64/DOUBLE             1
     UCOMISD_XMMsd_XMMsd         UCOMISD       SSE      SSE2         0        36         3        64/DOUBLE             1        64/DOUBLE             1        32/INT                1

Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_
Image:               matrix_multiplications.exe
Address:             0x403ef0
Calls:                        0
Instructions counts:          0
FLOP counts (TODO):           0

Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0
Image:               matrix_multiplications.exe
Address:             0x403510
Calls:                        1
Instructions counts:       2409
FLOP counts (TODO):           0
FLOP instructions:   
     XED_IFORM                XED_ICLASS   XED_CAT   XED_EXT    is_FMA    counts      #opd     opd_1/type       #elements     opd_2/type       #elements     opd_3/type       #elements
     ADDSD_XMMsd_XMMsd             ADDSD       SSE      SSE2         0       216         2        64/DOUBLE             1        64/DOUBLE             1
     MULSD_XMMsd_MEMsd             MULSD       SSE      SSE2         0       216         2        64/DOUBLE             1        64/DOUBLE             1

Routine (Procedure): _Z20multiplySparseMatrixP9cs_sparseS0_S0_.cold
Image:               matrix_multiplications.exe
Address:             0x40245a
Calls:                        0
Instructions counts:          0
FLOP counts (TODO):           0

Routine (Procedure): _Z14multiplyMatrixPPdPiS1_S0_S1_S1_S0_S1_S1_.part.0.cold
Image:               matrix_multiplications.exe
Address:             0x402450
Calls:                        0
Instructions counts:          0
FLOP counts (TODO):           0

```

## TODO
* Multi-threading Support
* Total FLOP computations

## Reference
* [Pin - A Dynamic Binary Instrumentation Tool](https://software.intel.com/content/www/us/en/develop/articles/pin-a-dynamic-binary-instrumentation-tool.html)
* [Pin: Pin 3.18 User Guide](https://software.intel.com/sites/landingpage/pintool/docs/98332/Pin/html/index.html)
* [X86 Encoder Decoder: X86 Encoder Decoder User Guide](https://intelxed.github.io/ref-manual/index.html)
* [Calculating “FLOP” using Intel® Software Development Emulator (Intel® SDE)](https://software.intel.com/content/www/us/en/develop/articles/calculating-flop-using-intel-software-development-emulator-intel-sde.html)
* [Roofline Performance Model](https://crd.lbl.gov/departments/computer-science/PAR/research/roofline/)