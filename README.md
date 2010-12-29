 econ-cam-test
=============

Modified version of e-con Systems 'feature-test' console app for evaluating 
their gumstix overo camera boards.

I have a e-CAM32_OMAP_GSTIX that I am using for testing.

There are no functional changes yet, though there are a couple of performance
improvements in the conversion routines.

I am working on the driver simultaneously and at some point in the future this
user program may no longer work with e-con's stock v4l2_driver.ko module.

I am also posting the driver module code 

http://github/scottellis/econ-cam-driver

I am actively working on both of these so your luck may vary. Right now it
all seems to work, but no doubt I've broken something.

I am working with the gumstix kernel linux-omap3-2.6.34r91.




