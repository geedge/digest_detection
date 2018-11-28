#!/bin/sh

PROG="dd"

while true; do
	mem_used=`ps -C $PROG u | grep -v RSS | awk '{print $6}'`
	
	if [ -z $mem_used ]; then
		mem_used=0
	elif [ $mem_used -gt 128000000 ]; then
		killall $PROG;
		echo "`date` -- $PROG used mem : $mem_used, kill it!" >> kill_by_mem.log
	fi
	
	sleep 10

done
