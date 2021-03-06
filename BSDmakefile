# Makefile

# Copyright (c) 2008, Natacha Porté
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

DEPDIR=depends
ALLDEPS=$(DEPDIR)/all
CFLAGS=-c -g -O3 -Wall -Wextra -Werror -fno-builtin -std=c99 -pedantic -DHAVE_CONFIG_H
LDFLAGS=-g -O3 -Wall -Wextra -Werror -fno-builtin -std=c99 -pedantic -DHAVE_CONFIG_H
CC=gcc

default:	ddns-client ddns-server

all:		GNUmakefile sha1-test \
		ddns-client ddns-server \
		stderr-client stderr-server

.PHONY:		default all clean


# Main project links

ddns-client:	client.o array.o csexp.o sha1.o message.o log-syslog.o utils.o\
		sensor.o
	$(CC) $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)

ddns-server:	server.o array.o csexp.o sha1.o message.o log-syslog.o utils.o\
		effector.o
	$(CC) $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)

stderr-client:	client.o array.o csexp.o sha1.o message.o log-stderr.o utils.o\
		sensor.o
	$(CC) $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)

stderr-server:	server.o array.o csexp.o sha1.o message.o log-stderr.o utils.o\
		effector.o
	$(CC) $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)

sha1-test:	sha1-test.o sha1.o
	$(CC) $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)

GNUmakefile:	BSDmakefile
		sed	-e 's/^\(all:.*\)GNUmakefile /\1/' \
			-e 's/\(rm .*\)GNUmakefile /\1/' \
			-e '/^GNUmakefile:/,/^$$/d' \
			-e 's/\$$(\.ALLSRC)/$$^/g' \
			-e 's/\$$(\.IMPSRC)/$$</g' \
			-e 's/\$$(\.OODATE)/$$?/g' \
			-e 's/\$$(\.MEMBER)/$$%/g' \
			-e 's/\$$(\.PREFIX)/$$*/g' \
			-e 's/\$$(\.TARGET)/$$@/g' \
			-e 's/^\.sinclude/-include/' \
			< $(.ALLSRC) > $(.TARGET)

clean:
	rm -f *.o
	rm -rf $(DEPDIR)
	rm -f GNUmakefile sha1-test ddns-client ddns-server \
		stderr-client stderr-server


# dependencies

.sinclude "$(ALLDEPS)"


# generic object compilations

.c.o:
	@mkdir -p $(DEPDIR)
	@touch $(ALLDEPS)
	@$(CC) -MM $(.IMPSRC) > $(DEPDIR)/$(.PREFIX).d
	@grep -q "$(.PREFIX).d" $(ALLDEPS) \
			|| echo ".include \"$(.PREFIX).d\"" >> $(ALLDEPS)
	$(CC) $(CFLAGS) -o $(.TARGET) $(.IMPSRC)

.m.o:
	@mkdir -p $(DEPDIR)
	@touch $(ALLDEPS)
	@$(CC) -MM $(.IMPSRC) > depends/$(.PREFIX).d
	@grep -q "$(.PREFIX).d" $(ALLDEPS) \
			|| echo ".include \"$(.PREFIX).d\"" >> $(ALLDEPS)
	$(CC) $(CFLAGS) -o $(.TARGET) $(.IMPSRC)
