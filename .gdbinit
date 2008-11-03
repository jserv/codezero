target remote localhost:1234
load final.axf
stepi
sym final.axf
break break_virtual
continue
stepi
sym /opt/codezero/tasks/mm0/mm0.axf
