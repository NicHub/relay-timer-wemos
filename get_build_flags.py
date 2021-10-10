#!/usr/bin/env python
# -*- coding: utf-8 -*-

import datetime
import os

UTC_Now = datetime.datetime.now()
date = UTC_Now.strftime( "%Y-%m-%d" )
time = UTC_Now.strftime( "%H:%M:%S" )
path = os.getcwd()

flags = '''
    -D COMPILATION_DATE=\\"%s\\"
    -D COMPILATION_TIME=\\"%s\\"
    -D PATH=\\"%s\\"
''' % (date, time, path)

print(flags)
