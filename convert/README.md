TTReg-Convert
=====

The reconstruction from structured-light images to point clouds is implemented by
the library: GCCS3DLib.lib, which is developed by another machine vision lab in siat.

we build a command-line exe from the library and invoke it to generate points in 
our TTReg-Compute package.

###Some Tips

Since our code is heavily dependent on the GCCS3DLib.lib, it may fails if the library
changes. So if there exists "unresolved external symbols" or anything wrong, try to 
communicate with their lab and find out where the problems are.
