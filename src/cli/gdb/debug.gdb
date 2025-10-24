set verbose off
set breakpoint pending on
set pagination off
set follow-exec-mode new
set follow-fork-mode child
py lotto_dir = r'%s' # format: python script dir
py sys.path.append(lotto_dir)
py from pylotto.util import init
py init(lotto_dir, r'%s', r'%s', r'%s', r'%s', r'%s') # format: file_filter, function_filter, addr2line, symbol_file, plugin_paths

#break main
#commands
#silent
#end
#run
#continue
#clear main

#set $last_pc = 0
#break sequencer_capture
#commands
#silent
#up-lotto
#py gdb.set_convenience_variable('new_pc', gdb.selected_frame().pc())
#if $new_pc == $last_pc
#continue
#else
#set $last_pc = $new_pc
#py print(f'Thread {gdb.selected_thread().global_num}.{gdb.selected_thread().num} "{gdb.selected_thread().name}" hit Breakpoint 2, {gdb.selected_frame().pc():#018x} in {gdb.selected_frame().name()} () at {gdb.find_pc_line(gdb.selected_frame().pc()).symtab}:{gdb.find_pc_line(gdb.selected_frame().pc()).line}')
#end
#end
#define hook-run
#inferior 1
#end

break recorder_end_trace
commands
silent
up-lotto
py print(f'Lotto replay finished')
py print(f'Thread {gdb.selected_thread().global_num}.{gdb.selected_thread().num} "{gdb.selected_thread().name}" hit Breakpoint 3, {gdb.selected_frame().pc():#018x} in {gdb.selected_frame().name()} () at {gdb.find_pc_line(gdb.selected_frame().pc()).symtab}:{gdb.find_pc_line(gdb.selected_frame().pc()).line}')
end
py import pylotto.handler as handler
define hook-next
py handler.should_finish = True
end
define hook-step
py handler.should_finish = True
end
#continue

load-plugin-symbols
