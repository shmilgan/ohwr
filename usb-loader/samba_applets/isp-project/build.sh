#!/bin/bash

## The ATMEL boot strap seems to only compile with a specific compiler
# Sourcery G++ Lite 2008q3-39 for ARM EABI
#
# ref: http://www.at91.com/forum/viewtopic.php/f,8/t,5352/

#### Function to compile each modules
compile_module()
{
	printf "\n\nCompiling module $1:\n"

	#Go to the mopdule directory
	cd ${root}/${1}

	# Obtain the git revision of the module
	GITR=`git log --abbrev-commit --pretty=oneline -1 . | cut -d" " -f1`

	# Append '+' symbol if some files need to be commited to git 	
	if [[ -n `git status -s .` ]]; then GITS='+'; else GITS=''; fi;	

	if [[ -n $2 ]]; then
		#Execute specific make commands such as make clean, make install
		make $2
	else
		#Compile the module
		make CROSS_COMPILE=${CROSS_COMPILE} CHIP=at91sam9g45 BOARD=at91sam9g45-ek MEMORIES=sram TRACE_LEVEL=5 DYN_TRACES=1 DEFINES="-D__GIT__=\\\"${GITR}${GITS}\\\" $DEFINES" INSTALLDIR=../../
	fi
}

#### Setup global variable
CROSS_COMPILE=/opt/wrs/misc/cd-g++lite/bin/arm-none-eabi-
root="$(dirname $(pwd)/$0)"

#### Check compiler
if [[ -z $(${CROSS_COMPILE}gcc --version | grep "Sourcery G++ Lite 2008q3-39") ]]; then
	echo "You should use Sourcery G++ Lite 2008q3-39 to compile samba-applets"
	echo "see reference: http://www.at91.com/forum/viewtopic.php/f,8/t,5352/"
	exit 1;
fi

#### Compilation of dataflash module
compile_module dataflash $1

#### Compilation of extern ram module
compile_module extram $1