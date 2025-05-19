
#CFLAGS := -I/usr/X11R6/include -I/usr/X11R6/include/freetype2 -ggdb3
#LDFLAGS := -L/usr/X11R6/lib -lxcb -lXau -lXdmcp -lfontconfig -lfreetype -lm

CFLAGS := -MMD -ggdb3 -I.
LDFLAGS := -L/usr/local/lib -L.

objs := fgit.o
scm_objs := scm/pack.o scm/util.o scm/errno.o

deps := $(objs:.o=.d) $(scm_objs:.o=.d)


fgit: $(objs) libscm.a
	cc $(LDFLAGS) -lscm $(objs) -o $@

libscm.a: $(scm_objs)
	ar rv $@ $(scm_objs)

.c.o:
	cc $(CFLAGS) -c -o $@ $<

clean:
	rm $(objs) $(deps)
	rm fgit

-include $(deps)
