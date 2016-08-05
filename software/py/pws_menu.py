#!/usr/bin/python
#coding:utf-8

from __future__ import print_function, unicode_literals
from functools import wraps
import os
import urlparse
import requests
import traceback
#from OpenSSL import SSL

from flask import (Flask,
                   make_response,
                   redirect,
                   render_template,
                   request,
                   session,
                   url_for
                  )

import submodule.dbox_tool

#
from logging import getLogger, StreamHandler, FileHandler, DEBUG, INFO, WARN, ERROR
logger = getLogger(__name__)
sh = StreamHandler()
sh.setLevel(INFO)
logger.setLevel(INFO)
logger.addHandler(sh)

#
app = Flask(__name__)
app.config['DEBUG'] = os.environ.get('DEBUG') == 'True'
app.secret_key = os.urandom(24)

#
CONF_FILENAME = ".dropbox_settings.conf"

APP_KEY_NAME = "APP_KEY"
APP_SECRET_NAME = "APP_SECRET"
ACCESS_TOKEN_STATE_NAME = "ACCESS_TOKEN_STATE"

ACCESS_TOKEN_STATE_OK = "ok"
ACCESS_TOKEN_STATE_NG = "ng"

#
def get_conf_pathname():
  return os.path.join(os.path.dirname(__file__), CONF_FILENAME)

#
def get_url(url):
  host = urlparse.urlparse(request.url).hostname
  return url_for(url,
                 _external=True,
                 _scheme='http' if host in ('127.0.0.1', 'localhost') else 'https')

#
@app.route("/")
def index():
  return render_template('menu.html')

#
@app.route("/setup_dropbox")
def setup_dropbox():
  app_key = ""
  access_token = ""
  params = {APP_KEY_NAME: "",
            ACCESS_TOKEN_STATE_NAME: ""
           }
  if request.args.has_key(APP_KEY_NAME) and request.args[APP_KEY_NAME]:
    params[APP_KEY_NAME] = request.args[APP_KEY_NAME]
    if request.args.has_key(ACCESS_TOKEN_STATE_NAME):
      params[ACCESS_TOKEN_STATE_NAME] = request.args[ACCESS_TOKEN_STATE_NAME]
  else:
    conf = submodule.dbox_tool.DropboxConfig(get_conf_pathname())
    params[APP_KEY_NAME] = conf.getAppKey()

  return render_template('dbox_setup.html', **params)

#
@app.route("/dropbox_auth_start")
def dropbox_auth_start():
  session[APP_KEY_NAME] = request.args[APP_KEY_NAME]
  session[APP_SECRET_NAME] = request.args[APP_SECRET_NAME]

  flow = submodule.dbox_tool.DropboxAuthFlow(session[APP_KEY_NAME], session[APP_SECRET_NAME], get_url("dropbox_auth_finish"), session)
  return redirect(flow.start())

#
@app.route("/dropbox_auth_finish")
def dropbox_auth_finish():
  flow = submodule.dbox_tool.DropboxAuthFlow(session[APP_KEY_NAME], session[APP_SECRET_NAME], get_url("dropbox_auth_finish"), session)
  try:
    access_token, user_id, url_state = flow.finish(request.args)

    conf = submodule.dbox_tool.DropboxConfig()
    conf.setAppKey(session[APP_KEY_NAME])
    conf.setAppSecret(session[APP_SECRET_NAME])
    conf.setAccessToken(access_token)

    connector = submodule.dbox_tool.DropboxConnector(conf)
    result = ACCESS_TOKEN_STATE_NG if connector.dbx is None else ACCESS_TOKEN_STATE_OK
  except:
    #traceback.print_exc()
    result = ACCESS_TOKEN_STATE_NG

  if result == ACCESS_TOKEN_STATE_OK:
    conf.save(get_conf_pathname())

  return redirect(url_for("setup_dropbox", **{APP_KEY_NAME: session[APP_KEY_NAME], ACCESS_TOKEN_STATE_NAME: result}))

#
@app.route("/cleanup_dropbox")
def cleanup_dropbox():
  return render_template('dbox_cleanup.html')

#
@app.route("/conf_cleanup")
def conf_cleanup():
  session.clear()
  try:
    os.remove(get_conf_pathname())
  except:
    #traceback.print_exc()
    pass
  return redirect(url_for("index"))

#
@app.route("/show_log")
def show_log():
  manager_log = ""
  uploader_log = ""
  try:
    with open("/pws/log/pws_manager.log", "r") as f:
      manager_log = f.read().decode("utf-8")
      manager_log.replace("<", "&lt;")
      f.close()
  except:
    #traceback.print_exc()
    pass

  try:
    with open("/pws/log/uploader.log", "r") as f:
      uploader_log = f.read().decode("utf-8")
      uploader_log.replace("<", "&lt;")
      f.close()
  except:
    #traceback.print_exc()
    pass

  return render_template('log.html', MANAGER_LOG=manager_log, UPLOADER_LOG=uploader_log)


#
if __name__ == "__main__":
  ssl = (os.path.join(os.path.dirname(__file__), "static/pws.crt"), os.path.join(os.path.dirname(__file__), "static/pws.key"))
  app.run(host="0.0.0.0", port=443, ssl_context=ssl)
  #app.run(debug=True, ssl_context=ssl)
