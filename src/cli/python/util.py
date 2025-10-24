import gdb
from pylotto.command import UpLotto, FinishLotto, FinishReplayLotto, RunReplayLotto, KakConnect, QlottoPrintLoc, QlottoPrintPc, LottoBreakAtClk, LottoBreakAtTime, LoadPluginSymbols
from pylotto.filter import ElidingLottoFilter
from pylotto.matcher import Matcher
from pylotto.handler import stop_handler


def init(lotto_path, file_filter, function_filter, addr2line, symbol_file,
         plugin_paths):
    file_regex = fr'^.*/libengine.so$|^{lotto_path}/.*${fr"|{file_filter}" if file_filter else ""}'
    function_regex = function_filter if function_filter else '$^'
    frame_matcher = Matcher(file_regex, function_regex)
    UpLotto(frame_matcher)
    FinishLotto(frame_matcher)
    FinishReplayLotto()
    RunReplayLotto()
    KakConnect()
    ElidingLottoFilter(frame_matcher)
    gdb.events.stop.connect(stop_handler)
    QlottoPrintLoc(addr2line, symbol_file)
    QlottoPrintPc()
    LottoBreakAtClk()
    LottoBreakAtTime()
    LoadPluginSymbols(plugin_paths)
