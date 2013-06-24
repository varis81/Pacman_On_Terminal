#include        <stdio.h>
#include        <sys/time.h>
#include        <signal.h>

set_ticker( )
{
        struct itimerval new_timeset;

        new_timeset.it_interval.tv_sec  = 0     ;
        new_timeset.it_interval.tv_usec = 40000;
        new_timeset.it_value.tv_sec   = 0  ;    
        new_timeset.it_value.tv_usec  = 40000 ; 

        return setitimer(ITIMER_REAL, &new_timeset, 0);
}

