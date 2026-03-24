#!/bin/sh
bindir=$(pwd)
cd /home/adminq/Bureau/M1/moteurdejeu/Glutony_lol/TP1_code/TP1/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "xYES" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		/usr/bin/gdb -batch -command=$bindir/gdbscript --return-child-result /home/adminq/Bureau/M1/moteurdejeu/Glutony_lol/TP1_code/build/TP1 
	else
		"/home/adminq/Bureau/M1/moteurdejeu/Glutony_lol/TP1_code/build/TP1"  
	fi
else
	"/home/adminq/Bureau/M1/moteurdejeu/Glutony_lol/TP1_code/build/TP1"  
fi
