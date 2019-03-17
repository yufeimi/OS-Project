# OS-Project
My OS project
My SRT preemption idea is : 

1. sort the pre_ready_queue get the 1st one's ID: ID_1.
2. update ready_queue.
3. sort the ready-queue get the 1st one's ID: ID_2.
if (ID_1==ID_2) will have preemption.
But I replaced one place in the RR code, I cannot find how to replace another place. So the RR preemption condition is still in the
SRT. We can talk about it later.

The SRT sort base on the remaining time is already there. The only thing left is the preemption condition now.

You can comment the SRT and run other parts. HOPE EVERYTHING WILL BE FINE.
