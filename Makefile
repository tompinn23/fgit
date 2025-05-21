
#CFLAGS := -I/usr/X11R6/include -I/usr/X11R6/include/freetype2 -ggdb3
#LDFLAGS := -L/usr/X11R6/lib -lxcb -lXau -lXdmcp -lfontconfig -lfreetype -lm

CFLAGS := -MMD -ggdb3 -I.
# -D_XOPEN_SOURCE=800
LDFLAGS := -L/usr/local/lib -L.

objs := fgit.o
scm_objs := scm/pack.o scm/util.o scm/errno.o scm/repository.o

deps := $(objs:.o=.d) $(scm_objs:.o=.d)


fgit: $(objs) $(scm_objs)
	cc $(LDFLAGS) $(objs) $(scm_objs) -lz -o $@

libscm.a: $(scm_objs)
	ar rcvs $@ $(scm_objs)

.c.o:
	cc $(CFLAGS) -c -o $@ $<

clean:
	-rm $(objs) $(scm_objs) $(deps)
	-rm fgit

-include $(deps)
