# libuninameslist
#
# Copyright (C) 2017, Shriramana Sharma
#
# This Python wrapper is subject to the same "BSD 3-clause"-type license
# which the wrapped C library is subject to.

'''
provides access to Unicode character names, annotations and block data

This Python wrapper implements:

1) three functions:
   name, annotation, block
2) one generator:
   blocks
3) one variable:
   version
4) two convenience functions:
   name2, uplus

`version` points to the internal version string of the library.
Run help() on the rest of the symbols for more info.
'''

__all__ = ["name", "annotation", "block",
           "blocks", "version",
           "name2", "uplus"]

# connecting to the dynamic library

from ctypes import *
from ctypes.util import find_library
_lib = CDLL(find_library("uninameslist"))

def _setSig(fn, restype, argtypes):
    if restype is not None: fn.restype = restype
    fn.argtypes = argtypes

# const char *uniNamesList_NamesListVersion(void);
_setSig(_lib.uniNamesList_NamesListVersion, c_char_p, [])
# const char *uniNamesList_name(unsigned long uni);
_setSig(_lib.uniNamesList_name, c_char_p, [c_ulong])
# const char *uniNamesList_annot(unsigned long uni);
_setSig(_lib.uniNamesList_annot, c_char_p, [c_ulong])
# int uniNamesList_blockCount(void);
_setSig(_lib.uniNamesList_blockCount, c_int, [])
# int uniNamesList_blockNumber(unsigned long uni);
_setSig(_lib.uniNamesList_blockNumber, c_int, [c_ulong])
# long uniNamesList_blockStart(int uniBlock);
_setSig(_lib.uniNamesList_blockStart, c_long, [c_int])
# long uniNamesList_blockEnd(int uniBlock);
_setSig(_lib.uniNamesList_blockEnd, c_long, [c_int])
# const char *uniNamesList_blockName(int uniBlock);
_setSig(_lib.uniNamesList_blockName, c_char_p, [c_int])

# internal helpers

class _block:
    '''Simple class containing the name, start and end codepoints of a Unicode block'''
    __slots__ = ["name", "start", "end"]

    def __init__(self, name, start, end):
        self.name = name; self.start = start; self.end = end

    def __str__(self):
        return "<‘{}’: {} - {}>".format(self.name, uplus(self.start), uplus(self.end))

    @staticmethod
    def _fromNum(num):
        return _block(_lib.uniNamesList_blockName(num).decode(),
                      _lib.uniNamesList_blockStart(num),
                      _lib.uniNamesList_blockEnd(num))

_blockCount = _lib.uniNamesList_blockCount()

# public symbols

'''documents the version of libuninameslist'''
version = _lib.uniNamesList_NamesListVersion().decode()

def name(char):
    '''returns the Unicode character name'''
    name = _lib.uniNamesList_name(ord(char))
    return "" if name is None else name.decode()

def name2(char):
    '''returns the normative alias if defined for correcting a Unicode character name, else just the name'''
    # NOTE: As of Unicode 10.0 these are only the 25 characters:
    #   U+01A2 U+01A3 U+0709 U+0CDE U+0E9D U+0E9F U+0EA3 U+0EA5 U+0FD0 U+11EC U+11ED U+11EE U+11EF
    #   U+2118 U+2448 U+2449 U+2B7A U+2B7C U+A015 U+FE18 U+FEFF U+0122D4 U+0122D5 U+01B001 U+01D0C5
    # ... but the function is provided if you want it.
    annot = annotation(char)
    try:
        # NOTE: index should not be needed as NamesList.txt always defines the normative alias first, but just keeping safe
        correctedNamePos = annot.index("\t% ") + 3
        try:
            nextTabPos = correctedNamePos + annot[correctedNamePos:].index("\n\t")  # there may be annotations beyond the alias
            return annot[correctedNamePos:nextTabPos]
        except ValueError:
            return annot[correctedNamePos:]
    except ValueError:
        return name(char)

def annotation(char):
    '''returns all Unicode annotations including aliases and cross-references as provided by NamesList.txt'''
    annot = _lib.uniNamesList_annot(ord(char))
    return "" if annot is None else annot.decode()

def blocks():
    '''a generator for iterating through all defined Unicode blocks'''
    for blockNum in range(_blockCount):
        yield _block._fromNum(blockNum)

def block(char):
    '''returns the Unicode block a character is in'''
    return _block._fromNum(_lib.uniNamesList_blockNumber(ord(char)))

def uplus(char):
    '''convenience function to return the Unicode codepoint for a character in the format U+XXXX for BMP and U+XXXXXX beyond that'''
    if type(char) is int:
        if not (0 <= char <= 0x10FFFF):
            raise ValueError("Invalid Unicode codepoint: U+{:X}".format(char))
        val = char
    else:
        val = ord(char)
    return ("U+{:06X}" if val > 0xFFFF else "U+{:04X}").format(val)

# test

def _test():
    print("Using libuninameslist version:\n\t", version)
    print("The Unicode name of ೞ is:\n\t", name("ೞ"))
    print("The Unicode annotation of ೞ is:")
    print(annotation("ೞ"))
    print("The Unicode name (with corrections) of ೞ is:\n\t", name2("ೞ"))
    print("The Unicode block of ೞ is:\n\t", block("ೞ"))
    print("The Unicode codepoint of ೞ is:\n\t", uplus("ೞ"))
    print()
    print("A complete list of blocks:")
    print("\n".join(str(block) for block in blocks()))

if __name__ == "__main__":
    _test()
