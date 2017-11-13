#-*- coding: utf-8 -*-

import os.path
import settings


def get_token_file_lines(token_file):
    """ 获取token文件行数，文件以\n换行 """
    fsize = os.path.getsize(token_file)
    return fsize / (settings.TOKEN_LENGTH + 1)


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
        if len(cols) == 3 and len(cols[0]) == settings.TOKEN_LENGTH:
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
            if len(cols) == 2 and len(cols[-1]) == settings.TOKEN_LENGTH:
                invalid_tokens.append(cols[-1])
        elif line.strip() == spec_head:
            is_match = True
    return invalid_tokens
