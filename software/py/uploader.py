#!/usr/bin/python
#coding:utf-8

from __future__ import print_function, unicode_literals

import threading
import os
import socket
import codecs
import struct
import time
import datetime
import json
import traceback

import submodule.oscmsg
import submodule.upload_queue

PWS_MANAGER_ADDR = str(socket.INADDR_LOOPBACK)
PWS_MANAGER_PORT = 8001
UPLOADER_RECEIVE_ADDR = str(socket.INADDR_LOOPBACK)
UPLOADER_RECEIVE_PORT = 8100

CONF_FILENAME = ".dropbox_settings.conf"
UPLOAD_FILE_SIZE_MAX = 140000000


#
from logging import getLogger, StreamHandler, FileHandler, DEBUG, INFO, WARN, ERROR
logger = getLogger(__name__)
sh = StreamHandler()
fh = FileHandler("/pws/log/uploader.log")
sh.setLevel(INFO)
fh.setLevel(INFO)
logger.setLevel(INFO)
logger.addHandler(sh)
logger.addHandler(fh)

#
def get_log_header():
  return "{0} {1}".format(datetime.datetime.now().strftime("%Y/%m/%d %H:%M:%S"), os.path.splitext(os.path.basename(__file__))[0])

#
def get_conf_pathname():
  return os.path.join(os.path.dirname(__file__), CONF_FILENAME)

#
class Uploader(threading.Thread):
  #
  def __init__(self, queue, data):
    threading.Thread.__init__(self)
    self.stopEvent = threading.Event()
    self.queue = queue
    self.data = data

  #
  def stop(self):
    self.stopEvent.set()

  #
  def run(self):
    logger.info("[{0}] enter Uploader with {1.pathname}.".format(get_log_header(), self.data))
    t1 = time.time()
    ret = self._upload()
    t2 = time.time()
    logger.info("[{0}] leave Uploader result = {1.pathname}, {2}, {3} sec elapsed.".format(get_log_header(), self.data, ret, (t2 - t1)))

  #
  def _upload(self):
    if self.data is None:
      logger.error("[{0}] bad upload-info".format(get_log_header()))
      self.queue.setStatus_NonBlock(self.data.pathname, submodule.upload_queue.UploadInfo.UPLOAD_STATE_ERROR)
      return False

    if not os.path.exists(self.data.pathname):
      logger.error("[{0}] file not found \"{1}\"".format(get_log_header(), self.data.pathname))
      self.queue.setStatus_NonBlock(self.data.pathname, submodule.upload_queue.UploadInfo.UPLOAD_STATE_ERROR)
      return False

    if UPLOAD_FILE_SIZE_MAX < os.path.getsize(self.data.pathname):
      logger.error("[{0}] file size limit over \"{1}\"".format(get_log_header(), self.data.pathname))
      self.queue.setStatus_NonBlock(self.data.pathname, submodule.upload_queue.UploadInfo.UPLOAD_STATE_ERROR)
      return False

    try:
      self.queue.setStatus_NonBlock(self.data.pathname, submodule.upload_queue.UploadInfo.UPLOAD_STATE_PROCESSING)
      if self._uploading(self.data.pathname):
        self.queue.setStatus_NonBlock(self.data.pathname, submodule.upload_queue.UploadInfo.UPLOAD_STATE_DONE)
      else:
        self.queue.setStatus_NonBlock(self.data.pathname, submodule.upload_queue.UploadInfo.UPLOAD_STATE_ERROR)
        return False
    except:
      traceback.print_exc()
      self.queue.setStatus_NonBlock(self.data.pathname, submodule.upload_queue.UploadInfo.UPLOAD_STATE_ERROR)
      return False

    return True

  #
  def _uploading(self, pathname):
    import submodule.dbox_tool
    conf = submodule.dbox_tool.DropboxConfig(get_conf_pathname())
    logger.debug("key: {0}\nsecret: {1}\ntoken: {2}".format(conf.getAppKey(), conf.getAppSecret(), conf.getAccessToken()))

    connector = submodule.dbox_tool.DropboxConnector(conf)
    return connector.uploadFile(pathname)

#
def sendUploadStart(pathname):
  logger.debug("UPLOAD START {0}".format(pathname))

  osc = submodule.oscmsg.OscMsg()
  osc.msg = "/uploader/upload/started"
  osc.params.append(pathname)
  packet = osc.build()

  logger.debug("send: {0}".format(codecs.encode(packet, "hex_codec")))

  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.sendto(packet, (PWS_MANAGER_ADDR, PWS_MANAGER_PORT))

#
def sendUploadResult(pathname, result, msg = ""):
  logger.debug("UPLOAD RESULT {0} = {1}".format(pathname, result))

  osc = submodule.oscmsg.OscMsg()
  osc.msg = "/uploader/upload/stopped"
  osc.params.append(result)
  osc.params.append(pathname)
  if msg:
    osc.params.append(msg)
  packet = osc.build()

  logger.debug("send: {0}".format(codecs.encode(packet, "hex_codec")))

  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.sendto(packet, (PWS_MANAGER_ADDR, PWS_MANAGER_PORT))

#
def startUpload(queue, data):
  data.uploader = Uploader(queue, data)
  data.uploader.start()

#
def queueUpdated(update_type, queue, data):
  if data is None:
    logger.debug("update ???:")
    return False

  if update_type == submodule.upload_queue.UploadInfo.UPDATE_TYPE_ADD:
    logger.debug("update ADD: {0.pathname}, {0.status}".format(data))
    if data.status != submodule.upload_queue.UploadInfo.UPLOAD_STATE_WAITING:
      return False
    startUpload(queue, data)
  elif update_type == submodule.upload_queue.UploadInfo.UPDATE_TYPE_REMOVE:
    logger.debug("update REMOVE: {0.pathname}, {0.status}".format(data))
  elif update_type == submodule.upload_queue.UploadInfo.UPDATE_TYPE_CHANGE:
    logger.debug("update UPDATE: {0.pathname}, {0.status}".format(data))
    if data.status == submodule.upload_queue.UploadInfo.UPLOAD_STATE_WAITING:
      startUpload(queue, data)
    elif data.status == submodule.upload_queue.UploadInfo.UPLOAD_STATE_DONE:
      sendUploadResult(data.pathname, 0)
      # return false, then remove data from queue
      return False
    elif data.status == submodule.upload_queue.UploadInfo.UPLOAD_STATE_ERROR:
      sendUploadResult(data.pathname, -1)
      # return false, then remove data from queue (don't retry)
      return False
  return True

#
class RequestReceiver(threading.Thread):
  CMD_UPLOAD_REQ = "/uploader/upload/start"

  #
  def __init__(self, addr, port):
    threading.Thread.__init__(self)
    self.queue = submodule.upload_queue.UploadQueue(queueUpdated)
    self.addr = addr
    self.port = port
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self.sock.bind((addr, port))

  #
  def run(self):
    logger.info("[{0}] enter RequestReceiver on {1}:{2}.".format(get_log_header(), self.addr, self.port))

    while True:
      data, addr = self.sock.recvfrom(1024)
      logger.debug("recv: {0}".format(codecs.encode(data, "hex_codec")))
      if 1 == len(data) and "q" == data[0]:
        break
      osc = submodule.oscmsg.OscMsg()
      osc.parse(data)
      if osc.msg != self.CMD_UPLOAD_REQ:
        logger.error("[{0}] bad message \"{1}\".".format(get_log_header(), osc.msg))
        sendUploadResult(osc.params[0], -1)
      elif 1 != len(osc.params):
        logger.error("[{0}] bad param count {1}.".format(get_log_header(), len(osc.params)))
        sendUploadResult(osc.params[0], -1)
      elif not isinstance(osc.params[0], basestring):
        logger.error("[{0}] bad param type.".format(get_log_header()))
        sendUploadResult(osc.params[0], -1)
      elif not self.queue.add(osc.params[0]):
        logger.error("[{0}] failed to add \"{1}\".".format(get_log_header(), osc.params[0]))
        sendUploadResult(osc.params[0], -1)
      else:
        sendUploadStart(osc.params[0])

    logger.info("[{0}] leave RequestReceiver.".format(get_log_header()))

#
def main():
  logger.info("[{0}] enter process.".format(get_log_header()))

  quitEvent = threading.Event()
  uploadEvent = threading.Event()

  reqReceiver = RequestReceiver(UPLOADER_RECEIVE_ADDR, UPLOADER_RECEIVE_PORT)
  reqReceiver.start()
  reqReceiver.join()

  logger.info("[{0}] leave process.".format(get_log_header()))

#
if __name__ == "__main__":
  main()
