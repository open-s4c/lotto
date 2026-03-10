import gdb
import subprocess
import os


class LoadPluginSymbols(gdb.Command):

    def __init__(self, plugin_dirs):
        super(LoadPluginSymbols, self).__init__("load-plugin-symbols",
                                                gdb.COMMAND_STACK)
        self.plugin_dirs = plugin_dirs

    def invoke(self, arg, from_tty):
        for dir in self.plugin_dirs.split(':'):
            for root, dirs, files in os.walk(dir):
                for file in files:
                    if file.endswith(".so"):
                        file_so = os.path.join(root, file)
                        gdb.execute(f'add-symbol-file "{file_so}"')


class UpLotto(gdb.Command):

    def __init__(self, frame_matcher):
        super(UpLotto, self).__init__("up-lotto", gdb.COMMAND_STACK)
        self.frame_matcher = frame_matcher

    def invoke(self, arg, from_tty):
        last_lotto_frame = None
        current_frame = gdb.selected_frame()
        while current_frame != None:
            if (self.frame_matcher.frame_matches(current_frame)):
                last_lotto_frame = current_frame
            current_frame = current_frame.older()
        current_frame = last_lotto_frame.older(
        ) if last_lotto_frame != None else None
        if (current_frame != None):
            current_frame.select()


class FinishLotto(gdb.Command):

    def __init__(self, frame_matcher):
        super(FinishLotto, self).__init__('finish-lotto', gdb.COMMAND_RUNNING)
        self.frame_matcher = frame_matcher

    def invoke(self, arg, from_tty):
        frame = gdb.selected_frame()
        if (not self.frame_matcher.frame_matches(frame)):
            return
        while frame.older() is not None and self.frame_matcher.frame_matches(
                frame.older()):
            frame = frame.older()
        frame.select()
        gdb.execute('finish')


class FinishReplayLotto(gdb.Command):

    def __init__(self):
        super(FinishReplayLotto, self).__init__('finish-replay-lotto',
                                                gdb.COMMAND_RUNNING)

    def invoke(self, arg, from_tty):
        command_until_breakpoint('continue')


class RunReplayLotto(gdb.Command):

    def __init__(self):
        super(RunReplayLotto, self).__init__('run-replay-lotto',
                                             gdb.COMMAND_RUNNING)

    def invoke(self, arg, from_tty):
        command_until_breakpoint('run')


def command_until_breakpoint(command):
    all_breakpoints = gdb.breakpoints() or []
    breakpoints = [
        b for b in all_breakpoints
        if b.is_valid() and b.enabled and b.location != 'recorder_end_trace'
        and b.visible == gdb.BP_BREAKPOINT
    ]
    for b in breakpoints:
        b.enabled = False
    gdb.execute(command)
    for b in breakpoints:
        b.enabled = True


class KakConnect(gdb.Command):

    def __init__(self):
        super(KakConnect, self).__init__("kak-connect", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        gdb.execute('set mi-async on')
        gdb.execute(f'new-ui mi3 /tmp/gdb_kak_{arg}/pty')


class QlottoPrintPc(gdb.Command):

    def __init__(self):
        super(QlottoPrintPc, self).__init__('qlotto-print-pc',
                                            gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        try:
            vcpu = int(arg)
        except ValueError as e:
            print(
                f'vcpu must be an integer between 1 and 4 (provided "{arg}")')
            return
        if vcpu < 1 or vcpu > 4:
            print(f'vcpu must be between 1 and 4 (provided "{arg}")')
            return
        gdb.execute(f't {vcpu+int(os.environ.get("LOTTO_VCPU_OFFSET", 2))}')
        print(
            gdb.parse_and_eval('current_cpu->env_ptr->pc').format_string(
                format='x'))


class QlottoPrintLoc(gdb.Command):

    def __init__(self, addr2line, symbol_files):
        super(QlottoPrintLoc, self).__init__('qlotto-print-loc',
                                             gdb.COMMAND_DATA)
        self.addr2line = addr2line
        self.symbol_files = symbol_files.split(':') if symbol_files else []

    def invoke(self, arg, from_tty):
        try:
            vcpu = int(arg)
        except ValueError as e:
            print(
                f'vcpu must be an integer between 1 and 4 (provided "{arg}")')
            return
        if vcpu < 1 or vcpu > 4:
            print(f'vcpu must be between 1 and 4 (provided "{arg}")')
            return
        gdb.execute(f't {vcpu+int(os.environ.get("LOTTO_VCPU_OFFSET", 2))}')
        address = gdb.parse_and_eval('current_cpu->env_ptr->pc').format_string(
            format='x')
        for file in self.symbol_files:
            output = subprocess.Popen(
                f'{self.addr2line} -e {file} {address}',
                shell=True,
                stdout=subprocess.PIPE).stdout.read().decode("utf-8")
            if output != '??:0':
                print(output)
                return
        print(f'could not resolve the address {address}')


class LottoBreakAtClk(gdb.Command):

    def __init__(self):
        super(LottoBreakAtClk, self).__init__('lotto-break-at-clk',
                                              gdb.COMMAND_BREAKPOINTS)

    def invoke(self, arg, from_tty):
        try:
            clk = int(arg)
        except ValueError as e:
            print(f'clock must be an unsigned integer (provided "{arg}")')
            return
        if clk < 0:
            print(f'clock must be an unsigned integer (provided "{arg}")')
            return
        gdb.execute(f'set environment LOTTO_DEBUG_CLK_BOUND={clk}')
        try:
            gdb.execute(f'call sequencer_set_clk_bound({clk})')
        except gdb.error as e:
            print(f'failed to set clk bound: {e}')
            pass
        gdb.Breakpoint('sequencer_clk_met', temporary=True, internal=True)


class LottoBreakAtTime(gdb.Command):

    def __init__(self):
        super(LottoBreakAtTime, self).__init__('lotto-break-at-time',
                                               gdb.COMMAND_BREAKPOINTS)

    def invoke(self, arg, from_tty):
        try:
            t = int(arg)
        except ValueError as e:
            print(
                f'time in nanosecond must be an unsigned integer (provided "{arg}")'
            )
            return
        if t < 0:
            print(
                f'time in nanoseconds must be an unsigned integer (provided "{arg}")'
            )
            return
        gdb.execute(f'set environment LOTTO_DEBUG_TIME_BOUND_NS={t}')
        try:
            gdb.execute(f'call sequencer_set_time_bound_ns({t})')
        except gdb.error as e:
            print(f'failed to set time bound: {e}')
            pass
        gdb.Breakpoint('sequencer_time_met', temporary=True, internal=True)
