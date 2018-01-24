#!/usr/bin/env python

from bottle import run
from ui import app

run(app, host="localhost", port="9090")
