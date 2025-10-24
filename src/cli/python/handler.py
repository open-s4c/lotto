import gdb
from pprint import pprint


def stop_handler(event):

    if isinstance(event, gdb.BreakpointEvent):
        [x.delete() for x in event.breakpoints if x.temporary]
    gdb.execute('finish-lotto')
