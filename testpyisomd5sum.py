#!/usr/bin/python3

import os
import pyisomd5sum

# create iso file
os.system("genisoimage -quiet . > testiso.iso")

# implant it
print("Implanting -> ", pyisomd5sum.implantisomd5sum("testiso.iso", 1, 0))

# do it again without forcing, should get error
print("Implanting again w/o forcing -> ", pyisomd5sum.implantisomd5sum("testiso.iso", 1, 0))

# do it again with forcing, should work
print("Implanting again forcing -> ", pyisomd5sum.implantisomd5sum("testiso.iso", 1, 1))

# check it
print("Checking -> ",pyisomd5sum.checkisomd5sum("testiso.iso"))

def callback(offset, total):
    print("%s - %s" % (offset, total))

print("Run with callback")
pyisomd5sum.checkisomd5sum("testiso.iso", callback)

def callback_abort(offset, total):
    print("%s - %s" % (offset, total))
    if offset > 500000:
        return True
    return False

print("Run with callback and abort after offset of 500000")
pyisomd5sum.checkisomd5sum("testiso.iso", callback_abort)

# clean up
os.unlink("testiso.iso")
