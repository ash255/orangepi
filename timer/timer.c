#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/major.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/clk.h>

/* ====================  For Debug  =============================== */
#define ENTER()              printk(KERN_DEBUG "%s()%d - %s\n", __func__, __LINE__, "Enter ...")
#define EXIT()               printk(KERN_DEBUG "%s()%d - %s\n", __func__, __LINE__, "Exit")
#define DEBUG(fmt, arg...)   printk(KERN_DEBUG "%s()%d - "fmt, __func__, __LINE__, ##arg)
#define INFO(fmt, arg...)    printk(KERN_INFO "%s()%d - "fmt, __func__, __LINE__, ##arg)
#define WARNING(fmt, arg...) printk(KERN_WARNING "%s()%d - "fmt, __func__, __LINE__, ##arg)
#define ERROR(fmt, arg...)   printk(KERN_ERR "%s()%d - "fmt, __func__, __LINE__, ##arg)

/* ===============  Register Define From Datasheet  =============== */
#define HSTIMER_BASE        (0x3005000)
#define HSTIMER_SZIE        (0x100)
#define HSTIMER_IRQ_EN      (0x00)
#define HSTIMER_IRQ_STAS    (0x04)
#define HSTIMER_CTRL        (0x20)
#define HSTIMER_INTV_LO     (0x24)
#define HSTIMER_INTV_HI     (0x28)
#define HSTIMER_CURNT_LO    (0x2C)
#define HSTIMER_CURNT_HI    (0x30)

/* ====================  Driver Define  =========================== */
#define MODULE_NAME "hstimer"

#define SET_BIT(src, pos)   ((src)|(1<<(pos)))
#define CLR_BIT(src, pos)   ((src)&(~(1<<(pos))))
#define GET_BIT(src, pos)   (((src)>>(pos))&1)

#define SET_VAL(src, val, pos, len)     (((src)&(~(((1<<(len))-1)<<(pos))))|((val)&((1<<(len))-1))<<(pos))
#define GET_VAL(src, pos, len)          (((src)>>(pos))&((1<<(len))-1))


/*  ==================== ioctrol command  ==================== */
#define IOC_MAGIC        'h'
#define IOCSINTR         _IOR(IOC_MAGIC, 0, uint32_t)   //get interrupt count
#define IOCGINTR         _IOR(IOC_MAGIC, 1, uint32_t)   //get interrupt count
#define IOCSCFG          _IOW(IOC_MAGIC, 2, uint32_t)   //set timer cfg
#define IOCSINTV         _IOW(IOC_MAGIC, 3, uint64_t)   //set timer interval
#define IOCGCUR          _IOR(IOC_MAGIC, 4, uint64_t)   //get current timer count
#define IOCCTL           _IOW(IOC_MAGIC, 5, uint32_t)   //control timer status on or off
#define IOCDBG           _IO(IOC_MAGIC, 6)              //printk debug message

struct sunxi_timer
{
    void __iomem *reg_base;
    struct platform_device *pf_device;
    struct class *dev_class;
    struct device *chr_dev;
    struct clk *ahb_clk;            //AHB1 clock gate for DMA
    
    volatile void __iomem *hst_reg;

    int major;
    int irq;
    int open_cnt;            //single instance
    volatile int intr_cnt;    //interrupt count
};

static struct sunxi_timer *g_hst = NULL;

static void write_timer_interval(struct sunxi_timer *pdev, uint64_t val)
{
    writel(val&0xFFFFFFFF, pdev->hst_reg + HSTIMER_INTV_LO);
    writel((val>>32)&0xFFFFFFFF, pdev->hst_reg + HSTIMER_INTV_HI);
}

static uint64_t read_current_timer_value(struct sunxi_timer *pdev)
{
    uint64_t lo, hi;
    lo = readl(pdev->hst_reg + HSTIMER_CURNT_LO);
    hi = readl(pdev->hst_reg + HSTIMER_CURNT_HI);
    return (hi<<32) | lo;
}

static irqreturn_t sunxi_timer_interrupt(int irq, void *dev_id)
{
    struct sunxi_timer *pdev = (struct sunxi_timer *)dev_id;
    
    //clear pending of timer interrupt
    writel(1, pdev->hst_reg + HSTIMER_IRQ_STAS);
    
    pdev->intr_cnt++;
    return IRQ_HANDLED;
}

static int sunxi_open(struct inode *inode, struct file *file)
{
    file->private_data = g_hst;

    if (g_hst->open_cnt > 0) 
    {
        DEBUG("High Speed Timer opened already\n");
        return 0;
    }
    g_hst->open_cnt++;
    return 0;
}

static int sunxi_release(struct inode *inode, struct file *file)
{
    struct sunxi_timer *pdev = file->private_data;

    if (--pdev->open_cnt) 
    {
        DEBUG("There is not really close, just return!\n");
        return 0;
    }
    return 0;
}

static long sunxi_ioctl(struct file *file, uint32_t cmd, unsigned long arg)
{
    struct sunxi_timer *phst = file->private_data;
    uint64_t cur_cnt;
    int ret = 0;
    uint32_t cfg;
    
    ENTER();
    switch (cmd) 
    {
        case IOCSINTR:
        {
            phst->intr_cnt = arg;
            break;
        }
        case IOCGINTR:
        {
            ret = put_user(phst->intr_cnt, (uint32_t __user *)arg);
            break;
        }
        case IOCSCFG:
        {
            writel(arg, phst->hst_reg + HSTIMER_CTRL);
            break;
        }
        case IOCSINTV:
        {
            write_timer_interval(phst, arg);
            break;
        }
        case IOCGCUR:
        {
            cur_cnt = read_current_timer_value(phst);
            ret = put_user(cur_cnt, (uint64_t __user *)arg);
            break;
        }
        case IOCCTL:
        {
            if(arg == 1)
            {
                cfg = readl(phst->hst_reg + HSTIMER_CTRL);
                writel(cfg | 2, phst->hst_reg + HSTIMER_CTRL);
                writel(cfg | 1, phst->hst_reg + HSTIMER_CTRL);
            }else
            {
                cfg = readl(phst->hst_reg + HSTIMER_CTRL);
                writel(cfg & (~1), phst->hst_reg + HSTIMER_CTRL);
            }
            break;
        }
        case IOCDBG:
        {
            INFO("HSTIMER_IRQ_EN: %X\n"
                 "HSTIMER_IRQ_STAS: %X\n"
                 "HSTIMER_CTRL: %X\n"
                 "HSTIMER_INTV_LO: %X\n"
                 "HSTIMER_INTV_HI: %X\n"
                 "HSTIMER_CURNT_LO: %X\n"
                 "HSTIMER_CURNT_HI: %X\n",
                 readl(phst->hst_reg + HSTIMER_IRQ_EN),
                 readl(phst->hst_reg + HSTIMER_IRQ_STAS),
                 readl(phst->hst_reg + HSTIMER_CTRL),
                 readl(phst->hst_reg + HSTIMER_INTV_LO),
                 readl(phst->hst_reg + HSTIMER_INTV_HI),
                 readl(phst->hst_reg + HSTIMER_CURNT_LO),
                 readl(phst->hst_reg + HSTIMER_CURNT_HI));
            break;
        }
        default:
            ERROR("Invalid iocontrol command!\n");
            break;
    }
    EXIT();
    return ret;
}

static const struct file_operations sunxi_fops = 
{
    .owner = THIS_MODULE,
    .llseek = noop_llseek,
    .unlocked_ioctl = sunxi_ioctl,
    .open = sunxi_open,
    .release = sunxi_release,
};

static int sunxi_probe(struct platform_device *pdev)
{
    int ret = 0;
    uint32_t cfg = 0;

    ENTER();

    g_hst = (struct sunxi_timer*)kzalloc(sizeof(struct sunxi_timer), GFP_KERNEL);
    if (g_hst == NULL) 
    {
        ERROR("kzalloc struct sunxi_timer fail!\n");
        EXIT();
        return -ENOMEM;
    }
    memset(g_hst, 0, sizeof(struct sunxi_timer));
    
    g_hst->hst_reg = ioremap(HSTIMER_BASE, HSTIMER_SZIE);
    if(g_hst->hst_reg == NULL)
    {
        ERROR("ioremap HSTimer reg fail!\n");
        ret = -EIO;
        goto emloc;
    }
    DEBUG("g_hst->hst_reg: %p\n", g_hst->hst_reg);
    g_hst->pf_device = pdev;
    
    
    // 1. enable AHB1 clock
    g_hst->ahb_clk = clk_get(&g_hst->pf_device->dev, "hstimer");
    if (!g_hst->ahb_clk) 
    {
        ERROR("No Clock To HSTimer!\n");
        goto eiom;
    }
    clk_prepare_enable(g_hst->ahb_clk);
    
    // 2. config timer parameter
    cfg = 0;
    cfg = CLR_BIT(cfg, 31);         //HS_TMR0_TEST
    cfg = CLR_BIT(cfg, 7);          //HS_TMR0_MODE
    cfg = SET_VAL(cfg, 0, 4, 3);    //HS_TMR0_CLK, default: 0
    cfg = CLR_BIT(cfg, 1);          //HS_TMR0_RELOAD
    cfg = CLR_BIT(cfg, 0);          //HS_TMR0_EN
    writel(cfg, g_hst->hst_reg + HSTIMER_CTRL);
    write_timer_interval(g_hst, 0);
    
    // 3. config timer interrupt
    writel(1, g_hst->hst_reg + HSTIMER_IRQ_EN);
    g_hst->irq = platform_get_irq(pdev, 0);
    if(g_hst->irq < 0)
    {
        ERROR("Get HST IRQ Failed\n");
        ret = -EINTR;
        goto eclk;
    }
    ret = request_irq(g_hst->irq, sunxi_timer_interrupt, IRQF_SHARED, dev_name(&pdev->dev), g_hst);
    if(ret < 0)
    {
        ERROR("Set Irq Callback Failed\n");
        ret = -EINTR;
        goto eclk;
    }
    
    // create character device
    g_hst->major = register_chrdev(0, MODULE_NAME, &sunxi_fops);
    if (g_hst->major < 0) 
    {
        ERROR("register_chrdev fail!\n");
        ret = -ENODEV;
        goto eirq;
    }
    g_hst->dev_class = class_create(THIS_MODULE, MODULE_NAME);
    if (IS_ERR(g_hst->dev_class)) 
    {
        ERROR("class_create Fail!\n");
        ret = -ENODEV;
        goto edev;
    }
    g_hst->chr_dev = device_create(g_hst->dev_class, NULL, MKDEV(g_hst->major, 0), NULL, MODULE_NAME);
    if (IS_ERR(g_hst->chr_dev)) 
    {
        ERROR("device_create fail!\n");
        ret = -ENODEV;
        goto ecla;
    }

    INFO("HSTIMER_IRQ_EN: %X\n"
         "HSTIMER_IRQ_STAS: %X\n"
         "HSTIMER_CTRL: %X\n"
         "HSTIMER_INTV_LO: %X\n"
         "HSTIMER_INTV_HI: %X\n"
         "HSTIMER_CURNT_LO: %X\n"
         "HSTIMER_CURNT_HI: %X\n",
         readl(g_hst->hst_reg + HSTIMER_IRQ_EN),
         readl(g_hst->hst_reg + HSTIMER_IRQ_STAS),
         readl(g_hst->hst_reg + HSTIMER_CTRL),
         readl(g_hst->hst_reg + HSTIMER_INTV_LO),
         readl(g_hst->hst_reg + HSTIMER_INTV_HI),
         readl(g_hst->hst_reg + HSTIMER_CURNT_LO),
         readl(g_hst->hst_reg + HSTIMER_CURNT_HI));

    platform_set_drvdata(pdev, g_hst);
    EXIT();
    return 0;
    
ecla:
    class_destroy(g_hst->dev_class);
edev:
    unregister_chrdev(g_hst->major, MODULE_NAME);
eirq:
    free_irq(g_hst->irq, g_hst);
eclk:
    clk_disable_unprepare(g_hst->ahb_clk);
eiom:
    iounmap(g_hst->hst_reg);
emloc:
    kfree(g_hst);
    g_hst = NULL;
    EXIT();
    return ret;

}

static int sunxi_remove(struct platform_device *pdev)
{
    device_destroy(g_hst->dev_class, MKDEV(g_hst->major, 0));
    class_destroy(g_hst->dev_class);
    unregister_chrdev(g_hst->major, MODULE_NAME);
    kfree(g_hst);
    g_hst = NULL;
    return 0;
}

static const struct of_device_id sunxi_match[] = 
{
    {.compatible = "allwinner,sunxi-hst",},
    {},
};

static struct platform_driver g_platform_driver = 
{
    .probe = sunxi_probe,
    .remove = sunxi_remove,
    .driver = 
    {
           .name = MODULE_NAME,
           .owner = THIS_MODULE,
           .pm = NULL,
           .of_match_table = sunxi_match,
    },
};

static int __init sunxi_init(void)
{
    return platform_driver_register(&g_platform_driver);
}

static void __exit sunxi_exit(void)
{
    platform_driver_unregister(&g_platform_driver);
}


module_init(sunxi_init);
module_exit(sunxi_exit);

MODULE_DEVICE_TABLE(of, sunxi_match);
MODULE_DESCRIPTION("High Speed Timer For Orangepi 3 lts");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MJS");
