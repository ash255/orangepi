查看全志芯片用户手册GIC一章，可查到High Speed Timer(以下简称HST)的硬件中断号。
从列表中可以看到HST的硬件中断号为78，但是在设备树中的中断号是按组划分的，所以设备树中填入的不是78。
从手册上的中断源表格中看到，前32项分别为SGI和PPI中断，所以设备树中的中断号要从32开始算，即HST在设备树中的中断号为78-32=46

为设备树文件添加以下结构
hstimer0x03005000 {
	#address-cells = <0x1>;
	#size-cells = <0x0>;
	compatible = "allwinner,sunxi-hst";
	reg = <0x0 0x3005000 0x0 0x400>;
	interrupts = <0x0 0x2E 0x4>;
	clocks = <0x2E>;
	status = "okay";
};

为hstimer字段为添加phandle属性
hstimer {
	#clock-cells = <0x0>;
	compatible = "allwinner,periph-clock";
	clock-output-names = "hstimer";
	linux,phandle = <0x2E>;
	phandle = <0x2E>;
};

high speed timer使用ahb1作为时钟源，正常情况下ahb1的时钟频率为200M
可以通过cat /sys/kernel/debug/clk/ahb1/clk_rate来查看当前ahb1的时钟速率

high speed timer的时钟频率计算方式为
hst_clk = 200M / interval / per-scale
其中interval可以随意指定，长度为56bit
而per-scale为时钟预分配参数，有效值有1、2、4、8、16，在HS_TMR0_CTRL_REG中指定