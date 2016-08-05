#!/usr/bin/python
#coding:utf-8

from __future__ import print_function, unicode_literals

import time
import types
import threading

#
class UploadInfo(object):
  UPLOAD_STATE_WAITING = 0
  UPLOAD_STATE_PROCESSING = 1
  UPLOAD_STATE_DONE = 2
  UPLOAD_STATE_ERROR = -1

  UPDATE_TYPE_ADD = 1
  UPDATE_TYPE_REMOVE = 2
  UPDATE_TYPE_CHANGE = 3

  #
  def __init__(self, pathname, status=UPLOAD_STATE_WAITING):
    object.__init__(self)
    self.pathname = pathname
    self.status = status
    self.entry_time = time.time()


#
class UploadQueue(object):
  #
  def __init__(self, update_callback):
    object.__init__(self)
    self.uploadQueue = []
    self.queueLocker = threading.Lock()
    self.update_callback = update_callback
    self.uploader = None

  #
  def __str__(self):
    s = "["
    for obj in self.uploadQueue:
      s += "["
      s += obj.pathname
      s += ", "
      s += str(obj.status)
      s += "]"
    s += "]"
    return s

  #
  def add(self, pathname):
    self.queueLocker.acquire()
    for obj in self.uploadQueue:
      if pathname == obj.pathname:
        self.queueLocker.release()
        return False
    ret = False
    info = UploadInfo(pathname)
    if isinstance(self.update_callback, types.FunctionType):
      if self.update_callback(UploadInfo.UPDATE_TYPE_ADD, self, info):
        self.uploadQueue.append(info)
        ret = True
    self.queueLocker.release()
    return ret

  #
  def remove(self, pathname):
    self.queueLocker.acquire()
    ret = self.remove_NonBlock(pathname)
    self.queueLocker.release()
    return False

  #
  def remove_NonBlock(self, pathname):
    for idx in range(len(self.uploadQueue)):
      if pathname == obj.pathname:
        info = self.uploadQueue.pop(idx)
        if isinstance(self.update_callback, types.FunctionType):
          self.update_callback(UploadInfo.UPDATE_TYPE_REMOVE, self, info)
        return True
    return False

  #
  def setStatus(self, pathname, status):
    self.queueLocker.acquire()
    ret = self.setStatus_NonBlock(pathname, status)
    self.queueLocker.release()
    return ret

  #
  def setStatus_NonBlock(self, pathname, status):
    for obj in self.uploadQueue:
      if pathname == obj.pathname:
        if obj.status != status:
          obj.status = status
          if isinstance(self.update_callback, types.FunctionType):
            if False == self.update_callback(UploadInfo.UPDATE_TYPE_CHANGE, self, obj):
              self.update_callback(UploadInfo.UPDATE_TYPE_REMOVE, self, obj)
              self.uploadQueue.remove(obj)
        return True
    return False
