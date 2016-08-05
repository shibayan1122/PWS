#!/usr/bin/python
#coding:utf-8

from __future__ import print_function, unicode_literals
import codecs
import struct

#
from logging import getLogger, StreamHandler, FileHandler, DEBUG, INFO, WARN, ERROR
logger = getLogger(__name__)
sh = StreamHandler()
sh.setLevel(WARN)
logger.setLevel(WARN)
logger.addHandler(sh)


#
class OscMsg(object):
  #
  def __init__(self):
    object.__init__(self)
    self.msg = ""
    self.params = []

  #
  def build(self):
    logger.debug("build osc msg: {0}".format(self.params))
    fmt = str("")
    data = str("")
    for val in self.params:
      if isinstance(val, int) or isinstance(val, long):
        fmt += str("i")
        wk = struct.pack(">i", val)
        data += struct.pack("{0}s".format(len(wk)), str(wk))
      elif isinstance(val, float):
        fmt += str("f")
        wk = struct.pack(">f", val)
        data += struct.pack("{0}s".format(len(wk)), str(wk))
      elif isinstance(val, basestring):
        fmt += str("s")
        data += str(val)
        data += str('\0')
        while 0 != len(data) % 4:
          data += str('\0')

    packet = struct.pack("{0}s".format(len(self.msg)), str(self.msg))
    packet += str('\0')
    while 0 != len(packet) % 4:
      packet += str('\0')

    if fmt:
      packet += str(',')
      packet += fmt
      packet += str('\0')
      while 0 != len(packet) % 4:
        packet += str('\0')

      packet += data

    return packet

  #
  def parse(self, data):
    idx, self.msg = self._parse_msg(data)
    idx, fmt = self._parse_fmt(data, idx)
    idx, self.params = self._parse_params(fmt, data, idx)
    logger.debug("> {0}, {1}, {2}, ({3})".format(self.msg, fmt, self.params, codecs.encode(data, "hex_codec")))

  # get message
  def _parse_msg(self, data):
    msg = ""
    for idx in range(len(data)):
      if '\0' == data[idx]:
        idx += 1
        break
      msg += data[idx]
    return ((idx + 3) / 4 * 4, msg)

  # get format
  def _parse_fmt(self, data, idx):
    fmt = ""
    if idx < len(data) and ',' == data[idx]:
      for idx in range(idx+1, len(data)):
        if '\0' == data[idx]:
          idx += 1
          break
        fmt += data[idx]
    return ((idx + 3) / 4 * 4, fmt)

  # get all params
  def _parse_params(self, fmt, data, idx):
    params = []
    for type in fmt:
      if 'i' == type:
        params.append(struct.unpack(">i", data[idx:idx+4])[0])
        #params.append(codecs.encode(data[idx:idx+4], "hex_codec"))
        idx += 4
      elif 'f' == type:
        params.append(struct.unpack(">f", data[idx:idx+4])[0])
        #params.append(codecs.encode(data[idx:idx+4], "hex_codec"))
        idx += 4
      elif 's' == type:
        wk = ""
        for idx in range(idx, len(data)):
          if '\0' == data[idx]:
            idx += 1
            break
          wk += data[idx]
        params.append(wk)
        idx = (idx + 3) / 4 * 4
      else:
        logger.error("Unknown Format \"{0}\"".format(type))
        break
    return (idx, params)
