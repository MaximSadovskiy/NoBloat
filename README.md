# Nobloat
A simple example how to write Windows programs with minimal size, without linking without system libs and even C standard library.

# How to run
```
clang -nostdlib -Os -static nobloat.c -o nobloat.exe -Wl,/entry:main,/NODEFAULTLIB
./nobloat.exe
```

This will create a very small executable without any bloat.
```
12.07.2026  12:56    <DIR>          .
12.07.2026  12:54             2 560 nobloat.exe
12.07.2026  12:54             3 581 nobloat.c
```