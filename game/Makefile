.PHONY: clean default flynshoot

ifeq ($(OS),Windows_NT)
CC = x86_64-w64-mingw32-gcc
AR = x86_64-w64-mingw32-ar

else
CC = gcc
AR = ar

endif

CFLAGS = -Wall -I ../port -I ../include
OFLAGS = -s -O3
LDFLAGS = -lncursesw -lpthread

default: flynshoot

flynshoot:
	$(CC) $(CFLAGS) $(OFLAGS) ../qep/source/*.c ../qf/source/*.c *.c $(LDFLAGS) -o $@;

clean:
	@exec echo -e "\n>> Cleaning... ";
	@find \( -name '*.[oa]' -or -name '*~*' -or -name '*.ali' -or -name '*.log' \
		-or -name 'flynshoot' -or -name 'flynshoot.exe' -or -name 'mprogram' -or -name 'a.out' \) -print -delete
	@exec echo ">> End Cleaning"
