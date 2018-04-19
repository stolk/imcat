# imcat

Preview any size image in a terminal window.


![Sample use](images/sampledesktop.png "Sample use.")

# Platform

Supports Linux, MacOS and Microsoft Windows 10 terminals.


## Introduction

Do you ever find yourself logged into a server, over SSH, but there is no X11 connection.
And you want to see the contents of an image?

imcat to the rescue!

imcat is a 24-bit image viewer that uses ANSI terminal colours to display any image supported by STB.
It automatically resizes to the width of your terminal, using proper sampling kernels.

imcat also works on the latest version of Windows 10.

## Todo

-

## Done

* Correct for aspect ratio of terminal font.

## Building

### Unix
On Linux, just use 'make' to build the imcat binary.

### Windows 10
On Windows, you need clang.exe from Visual Studio 2017 to build the imcat.exe binary. It's actually quite hard to get that compiler working, so you may just as well grab the pre-built <A HREF="https://stolk.org/imcat/imcat.exe">imcat.exe</A> binary.

## License

* Public Domain.

## Authors

[Bram Stolk](http://stolk.org).

[stb_image.h](http://nothings.org/stb_image.h) is by Sean Barrett et al.

## Acknoledgements

A shout out to [Frogtoss](http://github.com/mlabbe) for the idea and help. Thanks!

