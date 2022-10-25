# SIM
sim application for orangepi 3 lts

## Purpose
与SIM卡的一些小交互

## Support
orangepi 3 lts debian linux 4.9

## Usage
```
usage: ./sim [-rdch]
-c command mode
-r reset card
-d show debug log
-h print help
```

## Note
linux4.9版本中带有scr.ko，该驱动为智能卡驱动，相关代码在内核源码的字符驱动目录下。而5.10没有智能卡驱动。

## Driver
1. 修改sun50i-h6-orangepi-3-lts.dts设备树中的smartcard节点为以下状态，该文件可以在官方提供的linux内核中找到
```
smartcard@0x05005400 {
	#address-cells = <0x1>;
	#size-cells = <0x0>;
	compatible = "allwinner,sunxi-scr";
	device_type = "scr1";
	reg = <0x0 0x5005400 0x0 0x400>;
	interrupts = <0x0 0x9 0x4>;
	clocks = <0xb2 0xae>;
	clock-frequency = <0x16e3600>;
	//pinctrl-names = "default", "sleep";
	pinctrl-names = "default";
	//pinctrl-0 = <0xb3 0xb4>;
	pinctrl-0 = <0xb3>;
	//pinctrl-1 = <0xb5>;
	status = "okay";
```

2. 关闭spi设备，将spi节点改为以下状态，（只改了status字段）
```
spi@05011000 {
	#address-cells = <0x1>;
	#size-cells = <0x0>;
	compatible = "allwinner,sun50i-spi";
	device_type = "spi1";
	reg = <0x0 0x5011000 0x0 0x1000>;
	interrupts = <0x0 0xb 0x4>;
	clocks = <0x2 0x73>;
	clock-frequency = <0x5f5e100>;
	pinctrl-names = "default", "sleep";
	spi1_cs_number = <0x1>;
	spi1_cs_bitmap = <0x1>;
	status = "disabled";
	pinctrl-0 = <0xfc 0xfd>;
	pinctrl-1 = <0xfe 0xff>;

	spi_board0 {
		device_type = "spi_board0";
		compatible = "rohm,dh2228fv";
		spi-max-frequency = <0x124f80>;
		reg = <0x0>;
	};
};
```

3. 重新编译dts
dtc -I dts -O dtb -o sun50i-h6-orangepi-3-lts.dtb sun50i-h6-orangepi-3-lts.dts

4. 覆盖boot中的设备树文件
将sun50i-h6-orangepi-3-lts.dtb覆盖/boot/dtb/sunix/sun50i-h6-orangepi-3-lts.dtb

5. 重启

6. 如果成功可以用lsmod命令看到scr驱动已经加载，并且在/dev下存在smartcard设备节点；如果失败可以使用dmesg查看失败原因，dmesg | grep scr

## Pins
注意：由于我的sim模块没有detect管脚，所以板子出来detect管脚直接连接到GND上


![pins](https://github.com/ash255/SIM/blob/main/pins.png)

## Preview
![preview](https://github.com/ash255/SIM/blob/main/preview.jpg)
