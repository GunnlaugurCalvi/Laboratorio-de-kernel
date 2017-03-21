obj-m += kernellab.o
ccflags-y := -std=gnu99


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -Wall -std=gnu99 -o request-pid request-pid.c
	@echo
	@echo
	@echo "!!! Make sure that you remember to do \"make handin\" !!!"
	@echo
	@echo
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f request-pid

handin:
	@tar cf - . | ssh skel.ru.is "cat > /labs/sty16/.handin/kernellab/$$(cat .username)/kernellab.tar" > /dev/null 2>&1 
	@if [ $$? -ne 0 ]; then \
		echo "Handin did not work. Did you run ./setup.sh?"; \
	 else \
		echo "Kernellab handed in successfully."; \
	 fi
check:
	@echo -n "Last handin: "
	@ssh skel.ru.is "stat /labs/sty16/.handin/kernellab/$$(cat .username)/kernellab.tar" 2>&1 | grep Modify: | sed -e 's/Modify: //g' -e 's/\..*//g'
