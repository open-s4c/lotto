# Usage in own gdb scripts:
#
# source <lotto/scripts/qlotto-defines.gdb>
# # debugging a linux kernel
# lx-symbols

set tcp connect-timeout unlimited
set remotetimeout 99999

############################################################
### Workaround for gdb usage of vCont:c when doing stepi ###
define hook-stepi
monitor qlotto:hook-stepi
end

define hookpost-stepi
set $poststepi=1
end

define hook-stop
if $poststepi = 1
  monitor qlotto:hookpost-stepi
  set $poststepi = 0
end
end
### Workaround for gdb usage of vCont:c when doing stepi ###
############################################################

# Queries the gdb-server tick count, increasing with each executed instruction
define gdb-clock
monitor qlotto:gdb-clock
echo \n
end

# Queries the lotto clock, increasing with each lotto capture
define lotto-clock
monitor qlotto:lotto-clock
echo \n
end

# Sets a breakpoint for a specific gdb-server tick value
define break-gdb-clock
if $argc != 1
  echo "Argument for gdb clock value to break on missing!\n"
  return
end
monitor qlotto:break-gdb-clock:$arg0
end

# Sets a breakpoint for a specific lotto clock
define break-lotto-clock
if $argc != 1
  echo "Argument for lotto clock value to break on missing!\n"
  return
end
monitor qlotto:break-lotto-clock:$arg0
end

# Connect to qlotto gdb-server
target remote :12255

# Allows a breakpoint/step to hit on any core, not just the current one
set scheduler-locking off
