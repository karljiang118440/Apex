Building of examples for Linux OS
=================================

1) Set following system variables:
----------------------------------
  - export ARCH=arm
     - Target architecture
  - export PATH=/path/to/your/CHESS/compiler/binaries:$PATH
     - Path to the target APEX compiler binaries
  - export PATH=/path/to/your/arm/cross/compiler/binaries:$PATH
     - Path to the compiler binaries
     
2) Build the application
-----------------------
  BUILD WITHOUT APEX COMPILER
  ---------------------------
  - run "make" in corresponding build-v234ce-* directory
  
  REBUILDING THE APEX GRAPHS
  --------------------------
  - cd to corresponding graphs/build-apu-tct-sa-d directory
  - run "make allsub" command
 
 
 
 
 
 
 
 
 #1、TODO：
 
 ##1.1、 case_mobilenet .cpp 中有相关的图片测试，模型已经没问题，需要对相关视频进行检测，这一步需要在mobilenet_loop.cpp 中添加视频检测功能。
