objects = sim.o logger.o tlv.o

sim : $(objects)
	cc -o sim $(objects)
sim.o : sim.c sunxi-scr-user.h sim.h
	cc -c sim.c
logger.o : logger.c logger.h
	cc -c logger.c

tlv.o : tlv.c tlv.h
	cc -c tlv.c

.PHONY : clean
clean :
	-rm sim $(objects)