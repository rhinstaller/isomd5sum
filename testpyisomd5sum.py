#!/usr/bin/python3

import os
import pyisomd5sum

# Pass in the rc, the expected value and the pass_all state
# Returns a PASS/FAIL string and updates pass_all if it fails
def pass_fail(rc, pass_value, pass_all):
    if rc == pass_value:
        return ("PASS", pass_all)
    else:
        return ("FAIL", False)


# create iso file
os.system("genisoimage -quiet . > testiso.iso")

# implant it
(rstr, pass_all) = pass_fail(pyisomd5sum.implantisomd5sum("testiso.iso", 1, 0), 0, True)
print("Implanting -> %s" % rstr)

# do it again without forcing, should get error
(rstr, pass_all) = pass_fail(pyisomd5sum.implantisomd5sum("testiso.iso", 1, 0), -1, pass_all)
print("Implanting again w/o forcing -> %s" % rstr)

# do it again with forcing, should work
(rstr, pass_all) = pass_fail(pyisomd5sum.implantisomd5sum("testiso.iso", 1, 1), 0, pass_all)
print("Implanting again forcing -> %s" % rstr)

# check it
(rstr, pass_all) = pass_fail(pyisomd5sum.checkisomd5sum("testiso.iso"), 1, pass_all)
print("Checking -> %s" % rstr)

def callback(offset, total):
    print("    %s - %s" % (offset, total))

print("Run with callback, prints offset and total")
(rstr, pass_all) = pass_fail(pyisomd5sum.checkisomd5sum("testiso.iso", callback), 1, pass_all)
print(rstr)

def callback_abort(offset, total):
    print("    %s - %s" % (offset, total))
    if offset > 500000:
        return True
    return False

print("Run with callback and abort after offset of 500000")
(rstr, pass_all) = pass_fail(pyisomd5sum.checkisomd5sum("testiso.iso", callback_abort), 2, pass_all)
print(rstr)

# clean up
os.unlink("testiso.iso")

if pass_all:
    exit(0)
else:
    exit(1)
