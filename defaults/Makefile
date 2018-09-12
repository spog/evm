_SRCDIR_ := $(PWD)
_BUILDIR_ := $(_SRCDIR_)/.build
export _SRCDIR_ _BUILDIR_

TARGETS = install clean

.PHONY: all
all:
	make -C $(_SRCDIR_) -f $(_BUILDIR_)/Makefile

.PHONY: $(TARGETS)
$(TARGETS):
	make -C $(_SRCDIR_) -f $(_BUILDIR_)/Makefile $@

