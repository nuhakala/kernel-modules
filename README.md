
This folder contains some kernel modules that I have done for practice. They are
not necessarily very pretty, and there are most likely some silly mistakes and
other stuff that might seem weird. Description of modules can be found from the
source files.

They are developed against kernel 6.5.7. During the development the kernel
was compiled with gcc 13.2.1. Later I tested that the modules work also when
compiling with gcc 14.2.1

The used kernel was mostly unedited (apart from enabling kernel debugging
features), but for the weasel module I exported an extra variable from the
kernel, so it requires the provided patch at least.

