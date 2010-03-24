load/r '/home/bahadir/codezero/build/final.elf'
load/ni/np '/home/bahadir/codezero/build/kernel.elf'
bexec smp_start_cores
bexec idle_task
