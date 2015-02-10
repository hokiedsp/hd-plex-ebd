/* Microdia Eject Button Device Daemon.
 * 
 * Version:	2.0 (Feb. 1, 2015)
 *
 * Author:	Takeshi Ikuma (tikuma_at_hotmail_dot_com)
 * 
 * Summary: This daemon program first detects the HD-Plex's Eject Button Device
 *          which is listed as an hidraw device in the system. Then, it will run
 *          in a loop, polling for the eject button press event. When a button 
 *          press is detected, it executes 'eject' shell command.
 *
 *          This daemon assumes that the system is only equipped with one Eject
 *          Button device and one optical drive. The daemon may need to be 
 *          modified if a system does not meet these conditions.
 *
 * Version History:
 * 2.0 - (T. Ikuma, 2/1/2015)
 *        * Works with SATA-based optical drives
 *        * Complete new codebase.
 *        * Switches from custom libusb-based low-level USB data access to 
 *          utilizing the default hidraw driver.
 *        * Modern SATA-based drives are not compatible with ioctl(fd,CDROMEJECT)
 *          call to eject and behaves like SCSI devices. Using the EJECT system 
 *          shell command enables the device for the SATA drives as well as
 *          being more future proof.
 * 1.0 - original release (Arsalan Masood)
 *          
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <libudev.h>
#include <syslog.h>

#define VENDOR_ID_STR 	      "0c45"
#define PRODUCT_ID_STR 	      "7703"
#define INTERFACE_NUMBER_STR 	"01"
#define EJECT_COMMAND			"eject /dev/cdrom"

// function to return the hidraw file descriptor for the Eject Button Device
int open_usbhidraw(const char* vid_str, const char* pid_str, const char* inum_str, int flags)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev, *rawdev;
	struct udev_device *intf_dev; /* The device's interface (in the USB sense). */
	const char *path;
	int device_found = 0;
	int device_handle = -1;
	
	/* Create the udev object */
	udev = udev_new();
	if (!udev)
	{
		syslog(LOG_ERR, "ejectd: Error during udev_new()\n");
		return device_handle;
	}
	
	/* Create a list of the devices in the 'hidraw' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	/* For each item enumerated, print out its information.
	   udev_list_entry_foreach is a macro which expands to
	   a loop. The loop will be executed for each member in
	   devices, setting dev_list_entry to a list entry
	   which contains the device's path in /sys. */
	udev_list_entry_foreach(dev_list_entry, devices)
	{
		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		path = udev_list_entry_get_name(dev_list_entry);
		rawdev = udev_device_new_from_syspath(udev, path);
	
		/* The device pointed to by dev contains information about
		   the hidraw device. In order to get information about the
		   USB device, get the parent device with the
		   subsystem/devtype pair of "usb"/"usb_device". This will
		   be several levels up the tree, but the function will find
		   it.*/
		dev = udev_device_get_parent_with_subsystem_devtype(rawdev,"usb","usb_device");
		if (!dev)
		{
			syslog(LOG_ERR, "ejectd: Unable to find parent usb device: %s\n",path);
			continue;
		}
	
		/* Call get_sysattr_value() to check for idProduct and idVendor. */
		device_found = !(strcasecmp(udev_device_get_sysattr_value(dev,"idVendor"),vid_str) 
								|| strcasecmp(udev_device_get_sysattr_value(dev, "idProduct"),pid_str));

		if (device_found)
		{
			/* Get the interface number of the udev node. */
			intf_dev = udev_device_get_parent_with_subsystem_devtype(rawdev,"usb","usb_interface");
			if (intf_dev)
			{
				const char *str = udev_device_get_sysattr_value(intf_dev, "bInterfaceNumber");
				device_found = (str!=NULL) && strcmp(str, inum_str)==0;
				if (device_found) break; /* correct hidraw device found, stop traversing the list */
			}
			else
			{
				device_found = 0;
			}
		}
		
		/* If device not found, unreference the current device and go on to the next*/
		udev_device_unref(rawdev);
	}

	if (device_found)
	{
		syslog(LOG_ERR, "ejectd: Found Eject Button Device: %s\n",udev_device_get_devnode(rawdev));
	
		/* OPEN the device */
		device_handle = open(udev_device_get_devnode(rawdev), flags);
	
		/* Free the enumerator object */
		udev_unref(udev);
	}

	/* Free the enumeration */
	udev_enumerate_unref(enumerate);
	
	return device_handle;
}

/* as long as monitoring flag is true, the eject button is active */
int monitoring;

void stopnow(int sig)
{
	// handles all the program terminating signals
	// clearing monitoring flag causes to stop the loop in main()
	monitoring = 0;
}

int eject_cdrom()
{
	// Modern SATA-based CD-ROM does not respond to CDROMEJECT ioctl
	// and requires SCSI drive commands. While it can be implemented 
	// in C by copying lines from the source file of the eject system
	// command, it is more future proof to rely on the eject command.
	return system(EJECT_COMMAND);
}

int main(int argc, char* argv[])
{
	int res;
	char buf[2];
	sigset_t sigmask, orig_sigmask;
	struct pollfd fds[1];
	
	// run this program in the background
	if(daemon(0,0))
	{
		syslog(LOG_ERR, "ejectd: error during daemon().\n");
		return 1;
	}

	// initialize the monitoring flag and set stopnow as the signal handler for the program terminating signals
	monitoring = 1;
	signal(SIGTERM, stopnow);
	signal(SIGQUIT, stopnow);
	signal(SIGKILL, stopnow);
	signal(SIGINT, stopnow);

	// create signal mask so that ppoll() will be sensitive to activated signals
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGQUIT);
	sigaddset(&sigmask, SIGKILL);
	sigaddset(&sigmask, SIGINT);
	if (sigprocmask(SIG_BLOCK, &sigmask, &orig_sigmask) < 0)
	{
		syslog(LOG_ERR, "ejectd: Error during sigprocmask().\n");
		return 1;
	}
	
	// find and open the Eject Button Device
	fds[0].fd = open_usbhidraw(VENDOR_ID_STR, PRODUCT_ID_STR, INTERFACE_NUMBER_STR, O_RDONLY);
	if (fds[0].fd<0)
	{
		syslog(LOG_ERR, "ejectd: Failed to open the device.\n");
		return 1;
	}
	fds[0].events = POLLIN;
	
	// loop until monitoring is terminated by the system
	while (monitoring)
	{
		// poll for USB activity as well as system signals
		res = ppoll(fds, 1, NULL, &orig_sigmask);
		if(res == -1)
		{
			syslog(LOG_ERR, "ejectd: Error during ppoll()\n");
			continue;
		}

		// MCU sends in 2 bytes of information - buf[0]-key id, buf[1]-depressed/released
		res = read(fds[0].fd, buf, 2);
		if (res!=2)
		{
			syslog(LOG_ERR, "ejectd: read() failed to read 2 bytes of data.\n");
			continue;
		}

		// look for the button press (as opposed to button release)
		if (buf[0]==3 && buf[1]==1)
		{
			if (eject_cdrom()<0) syslog(LOG_ERR, "ejectd: Failed to eject optical drive\n");
		}
	}	

	// close the device connection	
	close(fds[0].fd);
	syslog(LOG_ERR, "ejectd: The interface to Eject Button Device is closed.\n");

	return 0;
}

