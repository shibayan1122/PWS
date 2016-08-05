#!/usr/bin/python
#coding:utf-8

from __future__ import print_function, unicode_literals
import os
import json
import time
import datetime
import traceback

import dropbox


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
class DropboxConfig(object):
  APP_KEY_NAME = "app_key"
  APP_SECRET_NAME = "app_secret"
  ACCESS_TOKEN_NAME = "access_token"

  #
  def __init__(self, config_filename=""):
    object.__init__(self)
    self.conf_data = {}
    self.load(config_filename)

  #
  def load(self, config_filename):
    self.conf_data = {}
    import json
    try:
      with open(config_filename, "r") as f:
        self.conf_data = json.load(f, "utf-8")
        f.close()
    except:
      logger.error("[{0}] throw exp at load conf-file.".format(get_log_header()))
      #traceback.print_exc()
      self.conf_data = {}
      return False
    return True

  #
  def save(self, config_filename):
    import json
    try:
      with open(config_filename, "w") as f:
        text = json.dumps(self.conf_data, ensure_ascii=False, indent=2)
        f.write(text.encode("utf-8"))
        f.close()
    except:
      logger.error("[{0}] throw exp at save conf-file.".format(get_log_header()))
      #traceback.print_exc()
      return False
    return True

  #
  def getAppKey(self):
    return self._getValue(DropboxConfig.APP_KEY_NAME)

  #
  def getAppSecret(self):
    return self._getValue(DropboxConfig.APP_SECRET_NAME)

  #
  def getAccessToken(self):
    return self._getValue(DropboxConfig.ACCESS_TOKEN_NAME)

  #
  def _getValue(self, key):
    if key in self.conf_data:
       return self.conf_data[key]
    return ""

  #
  def setAppKey(self, value):
    self.conf_data[DropboxConfig.APP_KEY_NAME] = value

  #
  def setAppSecret(self, value):
    self.conf_data[DropboxConfig.APP_SECRET_NAME] = value

  #
  def setAccessToken(self, value):
    self.conf_data[DropboxConfig.ACCESS_TOKEN_NAME] = value

#
class DropboxConnector(object):
  #
  def __init__(self, conf):
    object.__init__(self)
    try:
      token = conf.getAccessToken()
      self.dbx = dropbox.Dropbox(token)
    except:
      self.dbx = None

  #
  def uploadFile(self, pathname):
    if self.dbx is None:
      logger.warn("[{0}] no dbx object.".format(get_log_header()))
      return False

    mtime = os.path.getmtime(pathname)
    try:
      with open(pathname, "rb") as f:
        data = f.read()
        f.close()
    except:
      logger.error("[{0}] throw exp at read file.".format(get_log_header()))
      #traceback.print_exc()
      return False

    dstPathname = "/pws/" + os.path.basename(pathname)

    # it can upload file less than 150MB
    try:
      meta = self.dbx.files_upload(data,
                                   dstPathname,
                                   dropbox.files.WriteMode.overwrite,
                                   False,
                                   datetime.datetime(*time.gmtime(mtime)[:6]),
                                   True)
    except dropbox.exceptions.ApiError as err:
      logger.error("[{0}] API ERROR = {1}.".format(get_log_header(), err))
      return False
    except:
      logger.error("[{0}] throw exp at upload.".format(get_log_header()))
      traceback.print_exc()
      return False

    return True

#
class DropboxAuthFlow(object):
  #
  def __init__(self, appKey, appSecret, finish_url, session):
    object.__init__(self)
    self.flow = dropbox.oauth.DropboxOAuth2Flow(appKey,
                                                appSecret,
                                                finish_url,
                                                session,
                                                "dropbox-auth-csrf-token")

  #
  def start(self):
    return self.flow.start()

  #
  def finish(self, args):
    return self.flow.finish(args)

#
if __name__ == "__main__":
  conf = DropboxConfig("dbox.conf")
  logger.debug("[{0}] key: {1}\nsecret: {2}\ntoken: {3}".format(get_log_header(), conf.getAppKey(), conf.getAppSecret(), conf.getAccessToken()))

  connector = DropboxConnector(conf)
  connector.uploadFile("/Users/asano_m/Desktop/html/cgi/testfile_xx1.txt")
