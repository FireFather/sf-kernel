arch_cpu=x86-64-avx2
make --no-print-directory -j profile-build ARCH=${arch_cpu} COMP=mingw
strip sf-kernel.exe
mv sf-kernel.exe sf-kernel_x64_avx2.exe 
make clean

arch_cpu=x86-64-bmi2
make --no-print-directory -j profile-build ARCH=${arch_cpu} COMP=mingw
strip sf-kernel.exe
mv sf-kernel.exe sf-kernel_x64_bmi2.exe 
make clean
