import gdb
from pylotto.decorator import ElidingLottoDecorator


class ElidingLottoIterator:

    def __init__(self, ii, frame_matcher):
        self.input_iterator = ii
        self.frame_matcher = frame_matcher

    def __iter__(self):
        return self

    def next(self):
        frame = next(self.input_iterator)
        elided = []
        while self.frame_matcher.decorator_matches(frame):
            elided.append(frame)
            frame = next(self.input_iterator)
        return ElidingLottoDecorator(frame, [])

    def __next__(self):
        return self.next()
