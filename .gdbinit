target remote localhost:1234
load final.axf
stepi
sym final.axf
break bkpt_phys_to_virt
continue
stepi
sym start.axf

