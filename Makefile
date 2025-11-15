CC       = gcc
LIBS     =
CFLAGS   = -O2 -g -Wno-unused-result

.PHONY: all clean

all:    balong-usbdload ptable-injector loader-patch ptable-list ptable-editor usbloader-packer

clean:
	rm -f *.o
	rm -f balong-usbdload
	rm -f ptable-injector
	rm -f loader-patch
	rm -f ptable-list
	rm -f ptable-editor
	rm -f usbloader-packer

#.c.o:
#	$(CC) -o $@ $(LIBS) $^ qcio.o

balong-usbdload: balong-usbdload.o parts.o patcher.o exploit.o
	@gcc $^ -o $@ $(LIBS)

ptable-injector: ptable-injector.o parts.o
	@gcc $^ -o $@ $(LIBS)

loader-patch: loader-patch.o patcher.o
	@gcc $^ -o $@ $(LIBS)

ptable-list: ptable-list.o parts.o
	@gcc $^ -o $@ $(LIBS)

ptable-editor: ptable-editor.o parts.o
	@gcc $^ -o $@ $(LIBS)

usbloader-packer: usbloader-packer.o
	@gcc $^ -o $@ $(LIBS)
