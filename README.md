# hd-plex-ebd
Linux daemon for the HD-PLEX Eject Button Device

This daemon interacts with the HD-PLEX fanless PC enclosures with optical drives (e.g., H1.SODD, H5.SODD, and H10.SODD), enabling the USB-controlled optical drive eject button.

##Requirement: 
Linux kernel must support UDEV device manager and HIDRAW driver.

##Required package: 
libudev-dev

##Installation instructions for Ubuntu and its derivative

1. If you have a previous version of the program installed, run

   $ sudo make uninstall

before proceeding with building and installing.

2. Building this program requires a libudev-dev package. If not already installed, run

	$ sudo apt-get install libudev-dev
	
3. Run

	$ make
	$ sudo make install

To Uninstall, run

	$ sudo make uninstall

##Need this daemon for other Linux distro?
Please help out! 
