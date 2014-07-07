VM Start/Stop scripts README

This directory contains the scripts necessary to start and stop VMs.
The three scripts that should be run by the user are:

start_new_vm.sh
undefine_all_vms.sh
undefine_vm.sh

The other scripts are called by these 3, and should not normally be
called by the user.  Every script must be called as root for them to
work correctly.

start_new_vm.sh is used to start a new VM instance, and has 9 arguments:

./start_new_vm.sh <username> <public key file> <template_path> <num cores> <memory> <number of data interfaces> <vlan list> <data ip list> <vm number>

These 9 arguments work as follows:
username:  The username for the person setting up the experiment.
public key file:  The user's public key, which is normally located at ~/.ssh/id_rsa.pub
template path:  The path to the VM template image, normally at /KVM_Images/img/
num cores:  The number of CPU cores for the VM.
memory:  The amount of memory in MB for the VM.
number of data interfaces:  The total number of data interfaces for the VM.
vlan list:  The vlan numbers for each of the data interfaces in the VM.
data ip list:  The IP addresses for each of the data interfaces in the VM.
vm number:  The number of the VM on this machine.  For example the first vm for pc12core21 is vm12c21v01.  Its number is 1.

"vlan list" and "data ip list" are both files containing 1 line for
each data interface.  For example, if "vlan list" is:

2
3

And "data ip list" is:

192.168.0.1
192.168.1.1

Then the VM will have 2 data interfaces, one on vlan 2 with IP 
192.168.0.1 and one on vlan 3 with IP 192.168.1.1.

undefine_vm.sh is used to stop and undefine a VM.  It works with 1 argument,
the vm's name to be stopped:

./undefine_vm.sh <name>

Name should be the full name, such as "vm12c21v01" and not just the number
for the vm (1 in the case of vm12c21v01).  undefine_all_vms.sh can be used
without any arguments to call ./undefine_vm.sh for all vms on the machine.
