@echo off
if "%1" == "-E" (
    echo void free(void *ptr);
    echo void *malloc(unsigned long long size);
    echo int printf(const char *format, ...);
)
