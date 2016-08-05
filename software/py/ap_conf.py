#!/usr/bin/python
#coding:utf-8

from __future__ import print_function, unicode_literals

import sys
import os
import subprocess
import time
import datetime
import shutil
import socket
import struct
import traceback

import submodule.oscmsg


WITHOUT_APPLY = False                    #テスト時に実際にインターフェイスの設定を変更させないためのフラグ。Falseなら設定を行う。

SRC_CONF_FILE_NAME = "/wpa_supplicant.conf"
DST_CONF_FILE_NAME = "/etc/wpa_supplicant/wpa_supplicant.conf"

PWS_MANAGER_ADDR = str(socket.INADDR_LOOPBACK)
PWS_MANAGER_PORT = 8001

#
from logging import getLogger, StreamHandler, FileHandler, DEBUG, INFO, WARN, ERROR
logger = getLogger(__name__)
sh = StreamHandler()
fh = FileHandler("/pws/log/ap_conf.log")
sh.setLevel(INFO)
fh.setLevel(INFO)
logger.setLevel(INFO)
logger.addHandler(sh)
logger.addHandler(fh)

#
def get_log_header():
  return "{0} {1}".format(datetime.datetime.now().strftime("%Y/%m/%d %H:%M:%S"), os.path.splitext(os.path.basename(__file__))[0])

# find "/dev/sd**" and get pathname
def getMountedVolumeInfo():
  mount = subprocess.check_output(["mount"])
  mounts = mount.split("\n");

  info = []
  for data in mounts:
    wk = data.split()
    if 0 < len(wk) and wk[0].startswith("/dev/sd"):
      info.append([wk[0].strip(), wk[2].strip()])

  return info

# 
def waitConnect():
  while True:
    outputs = subprocess.check_output(["wpa_cli", "status"]).split("\n")
    for line in outputs:
      wk = line.split("=")
      if 2 == len(wk) and "wpa_state" == wk[0]:
        logger.debug("... {0}".format(line))
        if "COMPLETED" == wk[1]:
          return True

    time.sleep(1)

  return False

# 
def applyApSetting(srcFilename):
  ret = False
  if WITHOUT_APPLY:
    pass
  else:
    try:
      shutil.copyfile(srcFilename, DST_CONF_FILE_NAME)
      outputs = subprocess.check_output(["wpa_cli", "ifname"]).split()
      logger.debug("interface = {0}.".format(outputs))
      subprocess.check_call(["wpa_cli", "-i", outputs[-1], "reconfigure"])
      ret = waitConnect()
    except subprocess.CalledProcessError, e:
      logger.error("[{0}] apply failed on {1.cmd} = {1.returncode}.".format(get_log_header(), e))
    except:
      logger.error("[{0}] apply failed.".format(get_log_header()))
      traceback.print_exc()

  return ret

# 
def exitProcess(result, msg = ""):
  logger.info("[{0}] leave process with {1}, \"{2}\".".format(get_log_header(), result, msg))

  osc = submodule.oscmsg.OscMsg()
  osc.msg = "/ap_configurator/configure/configured"
  osc.params.append(result)
  if msg:
    osc.params.append(msg)
  packet = osc.build()

  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.sendto(packet, (PWS_MANAGER_ADDR, PWS_MANAGER_PORT))

  sys.exit(result)

# main
def main():
  logger.info("[{0}] enter process.".format(get_log_header()))
  while True:
    # 1. get volume-info
    vols = getMountedVolumeInfo()
    logger.info("VOLUEMS: {0}".format(vols))

    for vol in vols:
      fname = vol[1] + SRC_CONF_FILE_NAME
      if os.path.exists(fname):
        # 2. copy setting
        if applyApSetting(fname):
          exitProcess(0)
        else:
          exitProcess(-1, "APPLY ERROR")
      else:
        logger.debug("file not found \"{1}\"".format(fname))

    time.sleep(1)

  return

if __name__ == "__main__":
  main()
