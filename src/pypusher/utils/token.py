#-*- coding: utf-8 -*-

import anyjson as json
from rdmysql import Database, Expr
from models.default import Token
import settings

Database.configures.update(settings.MYSQL_CONFS)


def get_token_query(packages = [], conditions = {}):
    """ 获得一个对token数据表的查询 """
    query = Token().filter_by(is_invalid = 0)
    if packages:
        query.filter(Expr('package').within(packages))
    for field, value in conditions.items():
        opname = '='
        if ' ' in field:
            field, opname = field.split(' ', 1)
        query.filter(Expr(field).op(opname, value))
    return query


def prepare_token_file(query, token_file):
    """
    准备token文件（一般是读取数据库），返回总数
    格式为：每行一个token并以\n换行
    """
    files = {}
    for i in range(16):
        query.set_number(i)
        suffix = '.%x' % i
        filename = token_file + suffix
        counter = 0
        fh = open(filename, 'wb')
        for row in query.iter(['DISTINCT(`token`)']):
            fh.write('%s\n' % row['token'])
            counter += 1
        fh.close()
        files[filename] = counter
    fh = open(token_file, 'wb')
    fh.write(json.dumps(files))
    fh.close()
    return files


def set_invalid_tokens(tokens):
    """ 标记token为失效状态 """
    changes = {'is_invalid': 1}
    query = Token()
    for token in tokens:
        query.set_number(int(token[0], 16))
        query.update(changes, token = token)
    return len(tokens)
