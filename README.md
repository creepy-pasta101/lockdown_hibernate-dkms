# lockdown_hibernate-dkms

Resets the `LOCKDOWN_HIBERNATION` state so that you can hibernate to a swapfile in LUKS, even if you booted into `Secure Boot` mode (Which enables kernel lockdown in many mainstream distros).

So you can use `Secure Boot + LUKS encrypted swap + Hibernation`.

Most distro forum threads recommend turning off secure-boot to enable hibernation. However, this now makes your desktop vulnerable to evil-maid attacks. If you are a Laptop user, you will be in a conundrum, You could use hibernation to save power, than the shitty s2idle power state that drains your battery, or disable secure boot.

Compiling a patched kernel is also recommended to some users in some forums, however, its just a chore to do for every kernel version, especially if you have a low-end hardware.

Gemini was used to help in coding. (You can check out the source code ofcourse!)

## OS

Tested with Fedora 44. Kernel 7.0.8. x86_64 machine. Should work with ARM machines, however it isn't tested.

## Prerequisites

For security, I will assume you have set up hibernation in an encrypted swapfile/swap partition. (Ofcourse, without kernel lockdown/secure boot)
You can refer to the [Arch wiki](https://wiki.archlinux.org/title/Power_management/Suspend_and_hibernate#Hibernation) or your distro's docs for more info on how to set it up.

You now have to create and add your MOK to the shim :

Skip this step if you have a MOK key (NVIDIA users in some distros typically have them, if they use the proprietary drivers)
```
sudo dkms generate_mok
sudo mokutil --import /var/lib/dkms/mok.pub # Change this if dkms generated it elsewhere
```
It will prompt you for a temporary password. Enter it.
Reboot. It will boot into mokutil tool and Proceed to enroll your MOK Key as highlighted [here](https://docs.fedoraproject.org/en-US/quick-docs/mok-enrollment/#_enrolling_self_signing_key_after_reboot).

Now you need to point DKMS to sign the module with your MOK key. In `/etc/dkms/framework.conf`, add/uncomment -
```
mok_signing_key="/var/lib/dkms/mok.key" # Change these to point to your generated keys
mok_certificate="/var/lib/dkms/mok.pub"
```

You can now enable secure-boot in your BIOS settings.

## Install

Git clone this repo and copy the folder to `/usr/src/lockdown_hibernate-0.1.0`

```
cd /usr/src/lockdown_hibernate-0.1.0

sudo dkms add lockdown_hibernate/0.1.0

sudo dkms build lockdown_hibernate/0.1.0

sudo dkms install lockdown_hibernate/0.1.0
```

Once this is done, you will need to early-load this module and add `lockdown_hibernate.enable=1` parameter to the [kernel command-line](https://wiki.archlinux.org/title/Kernel_parameters). (You can search for how to do both on your distro's documentation). 

If your distro uses dracut you need to run `echo 'force_drivers+=" lockdown_hibernate "' | sudo tee /etc/dracut.conf.d/lockdown_hibernate.conf` to early-load the kernel module, and then regenerate the kernel image with `sudo dracut -fv`

Reboot. You can now test it with `systemctl hibernate` :-)

## Credits

Inspired from https://github.com/folays/flex_lockdown_reset.
Gemini was used to assist in code creation (kinda my first time tinkering with the kernel).
Thanks to whoever decided that we could not hibernate on LUKS if the kernel has booted into Secure Boot.
