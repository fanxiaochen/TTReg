TTReg
=====

TTReg —— the software package for VCC's scanning device based on turntable.

###Introduction

TTReg can be used to scan 3d objects and obtain fully constructed point cloud data, which is 
a specified software package for our particular scanning devices. 

The scanning device mainly consists of a turntable, a projector and a camera. It is based on
the principle of structured-light and the turntable setting. 

We can first capture images from 12 views by efforts of turntable and then reconstruct 
the whole 3d point cloud of the real object.

###Main Features

This software package mainly includes three parts: 

1) Convert: we're not experts about structured-light reconstruction which is a hot topic in machine vision. So we get
a library from another lab and convert the library(.lib) to executive program(.exe) for our use. In our program, by 
invoking the exe we compute the reconstruction process.

2) Capture: control the device and capture structured-light images.

3) Compute: main software. Use it to generate point clouds from images view by view and then register the 12 views to 
the whole point cloud of the object. Now the registration is a semiautomatic process with interactive assistance which is quite complicated.

More details will be found in XXX file.

###Third-Party Dependencies

Some third-party dependencies with recommended version is needed as follows:

1) Qt4.8; 

2) OpenSceneGraph 3.0; 

3) Point Cloud Library 1.7(other necessary dependencies have been integrated to PCL)

###Building and Running

TTReg can only run in windows environment because of some device problems. 

We use cmake to configure and generate project files which is more convenient for publication.




