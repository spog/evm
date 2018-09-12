#
# The "u2up-build" project build rules
#
# Copyright (C) 2017-2018 Samo Pogacnik <samo_pogacnik@t-2.net>
# All rights reserved.
#
# This file is part of the "u2up-build" software project.
# This file is provided under the terms of the BSD 3-Clause license,
# available in the LICENSE file of the "u2up-build" software project.
#

ifneq ($(_OBJDIR_),)
_OBJS_ := $(SRCS:%.c=$(_OBJDIR_)/%.o)
_OBJS1_ := $(SRCS1:%.c=$(_OBJDIR_)/%.o)
_OBJS2_ := $(SRCS2:%.c=$(_OBJDIR_)/%.o)
_OBJS3_ := $(SRCS3:%.c=$(_OBJDIR_)/%.o)
_OBJS4_ := $(SRCS4:%.c=$(_OBJDIR_)/%.o)
_OBJS5_ := $(SRCS5:%.c=$(_OBJDIR_)/%.o)

# pull in dependency info for *existing* .o files
-include $(_OBJS_:.o=.d)
-include $(_OBJS1_:.o=.d)
-include $(_OBJS2_:.o=.d)
-include $(_OBJS3_:.o=.d)
-include $(_OBJS4_:.o=.d)
-include $(_OBJS5_:.o=.d)

# generate dependency info
$(_OBJDIR_)/%.d: %.c
	$(CC) -MM $(CFLAGS) $*.c > $(_OBJDIR_)/$*.d
	@sed -i 's|^.*:|$(_OBJDIR_)\/&|' $(_OBJDIR_)/$*.d

# compile with dependency info
$(_OBJDIR_)/%.o: $(_OBJDIR_)/%.d
	$(CC) -c $(CFLAGS) $*.c -o $(_OBJDIR_)/$*.o

## compile and generate dependency info
#$(_OBJDIR_)/%.o: %.c
#	$(CC) -c $(CFLAGS) $*.c -o $(_OBJDIR_)/$*.o
#	$(CC) -MM $(CFLAGS) $*.c > $(_OBJDIR_)/$*.d
#	@sed -ie 's|^|$(_OBJDIR_)\/|' $(_OBJDIR_)/$*.d

endif
