target remote localhost:1234
load final.axf
stepi
sym final.axf
break break_virtual
continue
stepi
sym start.axf

