#-*- coding: utf-8 -*-

import sys
try:
    reload(sys)
    sys.setdefaultencoding('utf-8')
except AttributeError:
    pass  #没起作用


import time
import os, os.path
from subprocess import Popen, PIPE
from myutils import prepare_token_file, set_invalid_tokens
from settings import *


def get_token_file_lines(token_file):
    """ 获取token文件行数，文件以\n换行 """
    fsize = os.path.getsize(token_file)
    return fsize / (TOKEN_LENGTH + 1)
    
    
def remove_file_lines(token_file, lineno = 0, linestr = ''):
    """ 去掉文件前面的若干行 """
    fh = open(token_file, 'rb')
    if lineno and lineno > 0: #指定行（含）之前
        for i in xrange(lineno):
            fh.readline()
    else: #第一次出现指定的内容（含）之前
        linestr = linestr.strip()
        for line in fh.xreadlines():
            if line.strip() == linestr:
                break
    content = fh.read()
    fh.close()
    fh = open(token_file, 'wb')
    fh.write(content)
    fh.close()
    
    
def parse_out_log(lines):
    spec_head = 'ERROR apns ---> Invalid token:'
    dt_size = len('YYYY-mm-dd HH:MM:SS ')
    invalid_tokens = []
    cols, lineno = [], 0
    for line in lines:
        if len(line) <= dt_size:
            continue
        if not line[dt_size:].startswith(spec_head):
            continue
        line = line[len(spec_head) + dt_size:]
        cols = [c.strip() for c in line.strip().split()]
        if len(cols) == 3 and len(cols[0]) == TOKEN_LENGTH:
            invalid_tokens.append(cols[0])
    if len(cols) == 3:
        lineno = cols[-1].strip(')')
        if lineno.isdigit():
            lineno = int(lineno) + 1
    return invalid_tokens, lineno
    
    
def parse_err_log(lines):
    spec_head = 'Invalid tokens:'
    invalid_tokens, is_match = [], False
    for line in lines:
        if is_match:
            cols = [c.strip() for c in line.strip().split()]
            if len(cols) == 2 and len(cols[-1]) == TOKEN_LENGTH:
                invalid_tokens.append(cols[-1])
        elif line.strip() == spec_head:
            is_match = True
    return invalid_tokens
    
    
def apn_push(content, token_file, log_file,
            cert_file, passphrase = '', is_sandbox = False):
    """ 执行命令，等待结束，并分析错误输出 """
    #如果日志目录不存在，创建目录
    log_dir = os.path.dirname(log_file)
    if not os.path.exists(log_dir):
        os.makedirs(log_dir, 0755)
    #执行shell命令，获得标准输出和标准错误
    cmd = [PUSHER_BIN, '-T', token_file, '-o', log_file,
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
            time.sleep(SLEEP_SECS)
        else:
            break


if __name__ == '__main__':
    if len(sys.argv) >= 3:
        content = u'%s' % sys.argv[2]
    else:
        content = u'我们的APP有了新功能，请大家升级吧！'
    if len(sys.argv) >= 2:
        token_file = sys.argv[1]
    else:
        token_file = os.path.join(PROJECT_DIR, 'tokens.txt')
        prepare_token_file(token_file, channels = PRO_CHANNELS)
    kwargs = dict(
        log_file = LOG_FILE,
        cert_file = CERT_FILE,
        passphrase = CERT_PASS,
        is_sandbox = False
    )
    apn_main(content, token_file, **kwargs)
