import gdb
import re
from functools import reduce


class Matcher:

    def __init__(self, file_regex, function_regex):
        self.file_regex = file_regex
        self.function_regex = function_regex

    def decorator_matches(self, decorator):
        function = decorator.function()
        solib_file = gdb.solib_name(decorator.address())
        try:
            src_file = gdb.find_pc_line(decorator.address()).symtab.filename
        except (AttributeError):
            src_file = None
        try:
            obj_file = gdb.find_pc_line(
                decorator.address()).symtab.objfile.filename
        except (AttributeError):
            obj_file = None
        return self.__matches([([solib_file, src_file,
                                 obj_file], self.file_regex),
                               ([function], self.function_regex)])

    def frame_matches(self, frame):
        name = frame.name()
        try:
            function = frame.function().print_name
        except (AttributeError):
            function = None
        solib_file = gdb.solib_name(frame.pc())
        try:
            src_file = gdb.find_pc_line(frame.pc()).symtab.filename
        except (AttributeError):
            src_file = None
        try:
            obj_file = gdb.find_pc_line(frame.pc()).symtab.objfile.filename
        except (AttributeError):
            obj_file = None
        return self.__matches([([solib_file, src_file,
                                 obj_file], self.file_regex),
                               ([function, name], self.function_regex)])

    def __matches(self, strings_to_regex):
        return reduce(
            lambda x, y: x or y,
            map(
                lambda x: reduce(
                    lambda x, y: x or y,
                    map(
                        lambda s: s is not None and bool(
                            re.search(x[1], str(s))), x[0])),
                strings_to_regex))
