#!/bin/bash
fgrep 'model name' /proc/cpuinfo | sort -u | xargs
ncpus=`sort -u /sys/devices/system/cpu/cpu*/topology/physical_package_id | wc -l`
corepercpu=`sort -u /sys/devices/system/cpu/cpu*/topology/core_id | wc -l`
threadpercore=`lscpu | grep "Thread(s) per core:" | xargs`
threadpercore=${threadpercore##* }
total=$((ncpus * corepercpu * threadpercore))
echo "$ncpus physical CPUs, $corepercpu cores/CPU,\
 $threadpercore hardware threads/core = $total hw threads total"

runbench() {
  $* ./timesyscall
  $* ./timectxsw
  $* ./timetctxsw
}

echo '-- No CPU affinity --'
runbench

echo '-- With CPU affinity set to logical cpu0 on core#0 reserved --'
runbench taskset 0x1
