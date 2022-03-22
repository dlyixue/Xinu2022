/*  main.c  - main */

#include <xinu.h>
// running code in experiment 
void sndA(void) {
	while (clktime <= 10);
	printf("sndA time: %dms\n",proctab[getpid()].time_used);
	//printf("total time : %ds\n",clktime);
}

void sndB(void) {
	while (clktime <= 10);
	printf("sndB time: %dms\n",proctab[getpid()].time_used);
}

void sndC(void) {
	while (clktime <= 10);
	printf("sndC time: %dms\n",proctab[getpid()].time_used);
}
void sleepms_test(void){
	int32 t1 = count1000;
	printf("当前count: %d 当前preempt： %d\n",t1,preempt);
	while (count1000-t1<10){}//控制执行10ms
	sleepms(10);
	printf("当前count: %d 当前preempt： %d\n",count1000,preempt);
}
process	main(void)
{
	resume(create(sndA, 1024, 60, "process 1", 0));
	resume(create(sndB, 1024, 40, "process 2", 0));
	resume(create(sndC, 1024, 20, "process 3", 0));
	//resume(create(sleepms_test, 1024, 60, "process 4", 0));//用于测试spleems
	return OK;
}
