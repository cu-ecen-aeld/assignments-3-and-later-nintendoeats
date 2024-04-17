#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR" || { echo "${OUTDIR} could not be created or accessed."; exit 1; }

if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig #create the default kernel configuration
    make -j12 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    make -j12 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    make -j12 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
    
fi

    echo "Adding the Image in outdir"
    cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image "${OUTDIR}/Image"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

mkdir "${OUTDIR}/rootfs" 
cd "${OUTDIR}/rootfs" 

#Create necessary base directories
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
else
    cd busybox
fi


#Make and install busybox
make distclean
make defconfig
make -j12 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a "${OUTDIR}/rootfs/bin/busybox" | grep "program interpreter"
${CROSS_COMPILE}readelf -a "${OUTDIR}/rootfs/bin/busybox" | grep "Shared library"

cd "${OUTDIR}/rootfs"

cd $(type -p aarch64-none-linux-gnu-gcc | rev | cut -d'/' -f3- | rev)
cd aarch64-none-linux-gnu/libc

# Add library dependencies to rootfs
cp lib/ld-linux-aarch64.so.1  "${OUTDIR}/rootfs/lib"
cp lib64/libm.so.6            "${OUTDIR}/rootfs/lib64"
cp lib64/libresolv.so.2       "${OUTDIR}/rootfs/lib64"
cp lib64/libc.so.6            "${OUTDIR}/rootfs/lib64"

# Make device nodes
cd "${OUTDIR}/rootfs"
sudo mknod -m 666 dev/null    c 1 3
sudo mknod -m 600 dev/console c 5 1

# Clean and build the writer utility
cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu-

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp -r * "${OUTDIR}/rootfs/home"
cp -r ../conf "${OUTDIR}/rootfs"

# Chown the root directory
cd "${OUTDIR}/rootfs"
sudo chown -R root:root *

# create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio

