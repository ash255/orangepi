#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>

/*  ==================== ioctrol command  ==================== */
#define IOC_MAGIC        'h'
#define IOCSINTR         _IOR(IOC_MAGIC, 0, uint32_t)   //get interrupt count
#define IOCGINTR         _IOR(IOC_MAGIC, 1, uint32_t)   //get interrupt count
#define IOCSCFG          _IOW(IOC_MAGIC, 2, uint32_t)   //set timer cfg
#define IOCSINTV         _IOW(IOC_MAGIC, 3, uint64_t)   //set timer interval
#define IOCGCUR          _IOR(IOC_MAGIC, 4, uint64_t)   //get current timer count
#define IOCCTL           _IOW(IOC_MAGIC, 5, uint32_t)   //control timer status on or off
#define IOCDBG           _IO(IOC_MAGIC, 6)              //printk debug message

static uint32_t read_ahb1_clk()
{
    uint32_t ahb_clk;
    char buffer[64] = {0};
    int fd = open("/sys/kernel/debug/clk/ahb1/clk_rate", O_RDONLY);
    if(-1 == fd)
    {
        printf("open /sys/kernel/debug/clk/ahb1/clk_rate failed\n");
        return 0;
    }
    
    read(fd, buffer, sizeof(buffer));
    ahb_clk = strtoul(buffer, NULL, 10);
    close(fd);
    return ahb_clk;
}

int main()
{  
    int cnt = 10, i, ret;
    uint32_t intr_cnt, ahb_clk, per_scale, expect, step;
    uint64_t intv;
    int fd = open("/dev/hstimer", O_RDWR);
    if (-1 == fd)
    {
        printf("open /dev/hstimer failed! \n");
        return 0;
    }

    ahb_clk = read_ahb1_clk();
    if(ahb_clk > 1000000)
        printf("ahb1 clock: %d M\n", ahb_clk / 1000000);
    else if(ahb_clk > 1000)
        printf("ahb1 clock: %d K\n", ahb_clk / 1000);
    else
        printf("ahb1 clock: %d\n", ahb_clk);
    
    per_scale = 1;  //default in driver init function
    intv = 1000000;
    printf("per-scale: %d (1 is default value)\n"
           "interval: %d\n"
           "high speed timer clock: %d\n", 
           per_scale,
           intv,
           ahb_clk / 1 / intv);
    
    step = ahb_clk / 1 / intv;
    expect = 0;
    
    ioctl(fd, IOCSINTR, 0);         //清中断计数
    ioctl(fd, IOCSINTV, intv);      //设置高速时钟间隔
    ioctl(fd, IOCCTL, 1);           //开启高速时钟
    
    while(cnt--)
    {
        ret = ioctl(fd, IOCGINTR, &intr_cnt);
        printf("interrupt count: %d, expect: %d\n", intr_cnt, expect);
        //ioctl(fd, IOCDBG);
        expect += step;
        sleep(1);
    }
    
    ioctl(fd, IOCCTL, 0);
    close(fd);
    return 1;
}