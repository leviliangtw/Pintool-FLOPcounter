# Pintool-FLOPcounter
* This is a [Pin](https://software.intel.com/content/www/us/en/develop/articles/pin-a-dynamic-binary-instrumentation-tool.html) tool. 
* Measures the **`floating point operations (FLOP)`** in the given program. 
* The computation is based on the methodology in this article: [Calculating “FLOP” using Intel® Software Development Emulator (Intel® SDE)](https://software.intel.com/content/www/us/en/develop/articles/calculating-flop-using-intel-software-development-emulator-intel-sde.html). 

## Content
* **`flop_counter.cpp`**: find the `target image` and instrument the `target routines` to record execution counts and necessary informations. 
* **`matrix_multiplications.cpp`**: a simple program implementing `normal matrix multiplications` and `sparse matrix multiplications`. 

## Build & Execute
```
$ cd ${pin_root}/source/tools/
$ git clone https://github.com/leviliangtw/Pintool-FLOPcounter.git
$ cd Pintool-FLOPcounter/
$ make
$ pin -t ./obj-intel64/flop_counter.so -- ./obj-intel64/matrix_multiplications.exe
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