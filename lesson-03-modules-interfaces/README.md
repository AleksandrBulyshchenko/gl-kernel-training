## Lesson 03 - Kernel module interfaces

* Investigate how syscalls work
* Port procfs examples by Oleg Tsiliuric to kernel v4.13
  * examples.245.proc
  * Searching for linux kernel where API had been changed: git bisect
  * Usually changes are related to arguments of API functions/callbacks
* Port sysfs examples
