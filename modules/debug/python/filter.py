import gdb
from pylotto.iterator import ElidingLottoIterator


class ElidingLottoFilter:

    def __init__(self, matcher):
        self.name = "Eliding Lotto"
        self.enabled = True
        self.priority = 100
        self.matcher = matcher
        gdb.frame_filters[self.name] = self

    def filter(self, frame_iter):
        return ElidingLottoIterator(frame_iter, self.matcher)
