
# subdirectories we want to compile
SUBDIRS = \
	libwr \
	tools \

# all variables are exported
export

# All targets must install as well, as later builds use headers/libs
all:
	for d in $(SUBDIRS); do $(MAKE) -C $$d $@ || exit 1; done

clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d $@ || exit 1; done

install:
