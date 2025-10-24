from gdb.FrameDecorator import FrameDecorator


class ElidingLottoDecorator(FrameDecorator):

    def __init__(self, frame, elided_frames):
        super(ElidingLottoDecorator, self).__init__(frame)
        self.frame = frame
        self.elided_frames = elided_frames

    def elided(self):
        return iter(self.elided_frames)
