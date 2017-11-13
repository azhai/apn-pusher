#-*- coding: utf-8 -*-

import sys
try:
    reload(sys)
    sys.setdefaultencoding('utf-8')
except AttributeError:
    pass  #没起作用


import time
import os, os.path
import anyjson as json
from subprocess import Popen, PIPE
from utils.token import get_token_query, prepare_token_file, set_invalid_tokens
from utils.file import remove_file_lines, parse_err_log
import settings


def apn_push(content, token_file, log_file,
            cert_file, passphrase = '', is_sandbox = False):
    """ 执行命令，等待结束，并分析错误输出 """
    #如果日志目录不存在，创建目录
    log_dir = os.path.dirname(log_file)
    if not os.path.exists(log_dir):
        os.makedirs(log_dir, 0755)
    #执行shell命令，获得标准输出和标准错误
    cmd = [settings.PUSHER_BIN, '-T', token_file, '-o', log_file,
            '-c', cert_file, '-P', passphrase, '-m', content, '-v']
    if is_sandbox:
        cmd.append('-d')
    output, errput = Popen(cmd, stdout=PIPE, stderr=PIPE).communicate()
    #从标准错误中获取失效token
    lines = errput.splitlines()
    invalid_tokens = parse_err_log(lines)
    if invalid_tokens:
        last_invalid_token = invalid_tokens[-1]
        remove_file_lines(token_file, lineno = 0, linestr = last_invalid_token)
    return invalid_tokens


def apn_main(content, token_file, **kwargs):
    while 1:
        invalid_tokens = apn_push(content, token_file, **kwargs)
        if invalid_tokens:
            set_invalid_tokens(invalid_tokens)
            #过多错误token导致失败，需要间隔一段时间再连接APNs
            time.sleep(settings.SLEEP_SECS)
        else:
            break


def parser_cmd_args():
    app_name = sys.argv[1] if len(sys.argv) >= 2 else 'prod'
    kwargs = settings.APP_CONFS.get(app_name, {})
    packages = kwargs.pop('packages', [])
    conditions = kwargs.pop('conditions', {})
    content = kwargs.pop('content', u'')
    if len(sys.argv) >= 3:
        filename = sys.argv[2]
    else:
        filename = app_name + '.txt'
    if len(sys.argv) >= 4:
        content = u'%s' % sys.argv[3]
    opts =  {
        'app_name': app_name,
        'filename': filename,
        'content': content,
        'packages': packages,
        'conditions': conditions,
    }
    return opts, kwargs


if __name__ == '__main__':
    opts, kwargs = parser_cmd_args()
    #准备token
    token_file = os.path.join(settings.DATA_DIR, opts['filename'])
    if not os.path.exists(token_file):
        query = get_token_query(opts['packages'], opts['conditions'])
        files = prepare_token_file(query, token_file)
    else:
        with open(token_file, 'rb') as fh:
            files = json.loads(fh.read())
    #开始推送
    for file, counter in files.items():
        apn_main(opts['content'], file, **kwargs)
