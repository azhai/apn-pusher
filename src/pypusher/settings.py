#-*- coding: utf-8 -*-

import os.path

PROJECT_DIR = os.path.realpath(os.path.dirname(__file__))
LOGGING_DIR = os.path.join(PROJECT_DIR, 'logs')
CERTIFIC_DIR = os.path.join(PROJECT_DIR, 'certs')
DATA_DIR = os.path.join(PROJECT_DIR, 'data')

PUSHER_BIN = '/opt/bin/apn-pusher'
TOKEN_LENGTH = 64
LEAST_LINES = 20
SLEEP_SECS = 300


MYSQL_CONFS = {
    "default": {
        "drivername": "mysql+mysqldb",
        "host": "127.0.0.1",
        "port": 3306,
        "database": "test",
        "username": "dba",
        "password": "pass",
        "charset": "utf8",
    },
}


APP_CONFS = {
    'prod_m': {
        'log_file': os.path.join(LOGGING_DIR, 'prod.log'),
        'cert_file': os.path.join(CERTIFIC_DIR, 'prod.p12'),
        'passphrase': '888888',
        'is_sandbox': False,
        'packages': [],
        'conditions': {'gender': 'M'},
        'content': u'我们的APP有了新功能，请大家升级吧！',
    },
    'prod_f': {
        'log_file': os.path.join(LOGGING_DIR, 'prod.log'),
        'cert_file': os.path.join(CERTIFIC_DIR, 'prod.p12'),
        'passphrase': '888888',
        'is_sandbox': False,
        'packages': [],
        'conditions': {'gender': 'F'},
        'content': u'我们的APP有了新功能，请大家升级吧！',
    },
}
