#-*- coding: utf-8 -*-

from rdmysql import Archive


class Token(Archive):
    curr_has_suffix = True
    suffix_mask = '%x'
    
    __dbkey__ = 'default'
    __tablename__ = 't_tokens'
