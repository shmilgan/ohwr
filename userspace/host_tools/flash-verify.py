#!/usr/bin/python

# Simple script that verifies if all images stored to WR Switch match with
# firmware binaries
#
# Copyright (C) 2014, CERN.
#
# Author:      Grzegorz Daniluk <grzegorz.daniluk@cern.ch>
#
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import subprocess
import sys
import argparse
import paramiko
import hashlib
import time
import socket


########################
##   SOME CONSTANTS   ##
########################

#defualts
SSH_USER			=	"root"
SSH_PASSWD		= ""
MAC1_DEF			= "02:34:56:78:9A:BC"	# taken from flash-wrs
MAC2_DEF			= "02:34:56:78:9A:00"	# taken from flash-wrs
BAREBOX				= "barebox.bin"
AT91F_DEF			=	"at91bootstrap.bin"
BBF_DEF				=	"bb.bin"
#BBF_DEF				=	"barebox.bin"
IRAM_DEF      = "wrs-initramfs.gz"
ZIMG_DEF      = "zImage"

#others
AT91_MTD	= "/dev/mtd2"
BB_MTD		= "/dev/mtd3"
BOOT_VOL			= "ubi0:boot"
BOOT_MNTPT		= "/tmp/boot/"

#patterns to compare
AT91_IDX  = 0
BB_IDX		= 1
IRAM_IDX  = 2
ZIMG_IDX  = 3
orig_md5s = []
orig_sz		= []
orig_fnames = [AT91F_DEF, BBF_DEF, IRAM_DEF, ZIMG_DEF]
read_fnames = ["at91_r.bin", "bb_r.bin", "", ""]
read_devs		= [AT91_MTD, BB_MTD, BOOT_VOL]

########################
def verify_binaries(odir, host, passwd):
	print "Connecting to " + host
	client = paramiko.SSHClient()
	client.load_system_host_keys()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	#now make the connection
	try:
		client.connect(host, 22, SSH_USER, passwd)
	except socket.error:
		print "Could not connect to WR Switch, bye..."
		sys.exit(0)

	# 1. make dd of at91bootstrap.bin
	# 2. make dd of barebox
	for test_no in range(AT91_IDX, BB_IDX+1):
		sys.stdout.write("checking " + orig_fnames[test_no] + "...")
		sys.stdout.flush()
		cmd = "dd if=" + read_devs[test_no]
		stdin, stdout, stderr = client.exec_command(cmd)
		md5 = hashlib.md5(stdout.read(orig_sz[test_no])).hexdigest()
		stdin.close()
		stdout.close()
		stderr.close()
		if md5 == orig_md5s[test_no]:
			print "OK"
		else:
			print "Error"

	#prepare for next two:
	stdin, stdout, stderr = client.exec_command("mkdir " + BOOT_MNTPT)
	cmd = "mount -t ubifs " + BOOT_VOL + " " + BOOT_MNTPT # + ";sleep 1"
	stdin, stdout, stderr = client.exec_command(cmd)
	time.sleep(5)

	# 3. check wrs-initramfs.gz
	# 4. check zImage
	for test_no in range(IRAM_IDX, ZIMG_IDX+1):
		sys.stdout.write("checking " + orig_fnames[test_no] + "...")
		sys.stdout.flush()
		cmd = "md5sum " + BOOT_MNTPT + orig_fnames[test_no]
		stdin, stdout, stderr = client.exec_command(cmd)
		md5 = stdout.readline().split()[0]
		stdin.close()
		stdout.close()
		stderr.close()
		if md5 == orig_md5s[test_no]:
			print "OK"
		else:
			print "Error"

	cmd = "umount " + BOOT_MNTPT
	client.exec_command(cmd)
	client.exec_command("rmdir " + BOOT_MNTPT)

	# that one is special and different from previous ones
	# 5. Hardcoded mess, make it somehow nicer
	sys.stdout.write("checking /usr filesystem...")
	sys.stdout.flush()
	f = open("usr.tar", 'w')
	stdin, stdout, stderr = client.exec_command("tar -C /usr --exclude=var/lib/snmp -cvf - .")
	f.write(stdout.read())
	f.close()
	subprocess.call('mkdir -p temp_wr; tar -C temp_wr -xf usr.tar', shell=True)
	subprocess.call('find ./temp_wr -type f -print0 | xargs -0 md5sum > read.md5', shell=True)
	subprocess.call('rm -rf temp_wr/*', shell=True)
	#now the same for reference image
	subprocess.call('mkdir -p temp_wr; tar -C temp_wr -xzf '+odir+'wrs-usr.tar.gz', shell=True)
	subprocess.call('find ./temp_wr -type f -print0 | xargs -0 md5sum > orig.md5', shell=True)
	sz, md5_orig = anl_file("orig.md5")
	sz, md5 = anl_file("read.md5")
	if md5 == md5_orig:
		print "OK"
	else:
		print "Error"
	# time for a clean-up
	subprocess.call('rm -rf temp_wr usr.tar orig.md5 read.md5', shell=True)

	# disconnect from WRS when all is done
	client.close()

########################
# makes bb.bin from barebox.bin based on _m1_ and _m2_ MACs the same way
# flash-wrs does it
def make_bb_bin(odir, m1, m2):
	f = open(odir+BAREBOX, 'r')
	f2 = open(odir+BBF_DEF, 'w')
	for line in f:
		line = line.replace(MAC1_DEF, m1)
		line = line.replace(MAC2_DEF, m2)
		f2.write(line)
	f.close()
	f2.close()

# returns the size (in bytes) and md5 hash string of a file
def anl_file(filename):
	f = open(filename, 'r')
	f.seek(0, os.SEEK_END)
	size = f.tell()	#size of the file
	f.seek(0, os.SEEK_SET)
	md5 = hashlib.md5(f.read()).hexdigest() 	#md5 of the file
	f.close()
	return (size, md5)

# fills orig_md5s & orig_sz arrays with sizes and md5 of orig files
def read_orig_files(odir):
	for i in range(0, len(orig_fnames)):
		sz, md5 = anl_file(odir+orig_fnames[i])
		orig_sz.insert(i, sz)
		orig_md5s.insert(i, md5)

def main():
	parser = argparse.ArgumentParser(description='Tool for verifying images flashed to WR Switch')
	parser.add_argument('--ip', default="192.168.3.18", help='IP address of the WR Switch')
	parser.add_argument('--passwd', default=SSH_PASSWD, help='SSH password for the device')
	parser.add_argument('--odir', default="orig/", help='directory with files to be verified')
	parser.add_argument('--m1', default="", help='mac 1, must be the same as m1 in flash-wrs')
	parser.add_argument('--m2', default="", help='mac 2, must be the same as m2 in flash-wrs')
	args = parser.parse_args()

	# first create modified barebox.bin copy the same way flash-wrs does it
	make_bb_bin(args.odir, args.m1, args.m2)
	read_orig_files(args.odir)
	verify_binaries(args.odir, args.ip, args.passwd)


if __name__ == '__main__':
	main()
