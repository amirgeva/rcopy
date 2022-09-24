# rcopy - Resuming File Copy

A small utility that copies files / directories with the ability to resume
from where you left off, in case of a disconnect or any other failure.

Usage:

	rcopy [options] <source> <destination>
	
	-r  Enables directory recursion
	-c  Clears that cache database and basically starts over
	
source can be either a file or a directory
destintation can be a file if source is a file, or it can be the target directory

When copying a directory, the default is not to recurse.

The program maintains a checksum database to know what parts of the files are already copied,
so that a restart of the program will continue where it left off.

## How to build

	git clone --recurse-submodules https://github.com/amirgeva/rcopy
	cd rcopy
	mkdir build
	cd build
	cmake ..
	
Under Linux, use make to build rcopy.  On Windows with Visual Studio, open the solution and build.