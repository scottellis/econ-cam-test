 econ-cam-test
=============

Modified version of e-con Systems 'feature-test' console app for evaluating 
their gumstix overo camera boards.

I have a e-CAM32_OMAP_GSTIX that I am using for testing.

There are no functional changes yet, though there are a couple of performance
improvements in the conversion routines.

I am working on the driver simultaneously and at some point in the future this
user program may no longer work with e-con's stock v4l2_driver.ko module but
only my forked driver.

The forked driver module can be found here

http://github/scottellis/econ-cam-driver

I am actively working on both of these so your luck may vary. Right now it
all seems to work, but no doubt I've broken something. Give it a week or so.

Functions with the boilerplate headers still on them (hate that) have not been
changed (too much). Even some where I removed the header only got a quick look.

I am testing this stuff with an overo linux-omap3-2.6.34r91 kernel and my 
modified driver.





