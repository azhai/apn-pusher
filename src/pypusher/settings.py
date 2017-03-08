#-*- coding: utf-8 -*-

import os.path

PROJECT_DIR = os.path.realpath(os.path.dirname(__file__))
LOG_FILE = os.path.join(PROJECT_DIR, 'logs/push.log')
CERT_FILE = os.path.join(PROJECT_DIR, 'certs/prod.p12')
CERT_PASS = '123456'

PUSHER_BIN = '/usr/bin/apn-pusher'
TOKEN_LENGTH = 64
LEAST_LINES = 20
SLEEP_SECS = 300


PRO_CHANNELS = []
