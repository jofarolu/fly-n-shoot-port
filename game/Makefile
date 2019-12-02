.PHONY: clean default flynshoot.exe

CC = mgcc
AR = ar
STRIP = strip
CFLAGS = -Wall -Wextra -I$(MARTE)/arch/drivers/svga -I ../port -I ../include
OFLAGS = -s -Os -DNDEBUG
LDFLAGS = -lsvga

default: flynshoot.exe

flynshoot.exe:
	$(CC) $(CFLAGS) $(OFLAGS) ../qep/source/*.c ../qf/source/*.c *.c $(LDFLAGS) -o $@;
	$(STRIP) --strip-all $@

clean:
	@exec echo -e "\n>> Cleaning... ";
	@find \( -name '*.[oa]' -or -name '*~*' -or -name '*.ali' -or -name '*.log' \
		-or -name '*.exe' -or -name 'mprogram' -or -name 'a.out' \) -print -delete
	@exec echo ">> End Cleaning"
