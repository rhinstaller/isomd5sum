# isomd5sum

isomd5sum provides a way of making use of the ISO9660 application data area to
store md5sum data about the iso.  This allows you to check the iso given
nothing more than the iso itself.

## Build instructions

```
sudo dnf install -y popt-devel python3-devel
mkdir -p build/
cd build/
cmake -DCMAKE_INSTALL_PREFIX:PATH=$PWD -DPYTHON_VERSION=3 ..
make install
```

## Release instructions

```
./scripts/release X.Y.Z
```

You can send questions, enhancements, etc to
[anaconda-devel-list@redhat.com](mailto:anaconda-devel-list@redhat.com) or open
an issue.
