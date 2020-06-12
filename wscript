#! /usr/bin/env python
# encoding: utf-8

'''
@author: Milos Subotic <milos.subotic.sm@gmail.com>
@license: MIT

'''

###############################################################################

import os
import sys
import fnmatch
import shutil
import datetime
import glob

import waflib

###############################################################################

APPNAME = 'LPRS2_GPU_Emulator'

top = '.'

###############################################################################

def prerequisites(ctx):
	ctx.recurse('emulator')
	
	if sys.platform.startswith('linux'):
		# Ubuntu.
		ctx.exec_command2('apt-get -y install python3-pil')
	elif sys.platform == 'win32' and os.name == 'nt' and os.path.sep == '/':
		# MSYS2 Windows /mingw32/bin/python.
		ctx.exec_command2(
			'pacman --noconfirm -S mingw-w64-i686-python-pillow'
		)

def options(opt):
	opt.load('compiler_c compiler_cxx')
	
	opt.recurse('emulator')

def configure(conf):
	conf.load('compiler_c compiler_cxx')
	
	conf.recurse('emulator')

	conf.env.append_value('CFLAGS', '-std=c99')
	
	conf.find_program(
		'python3',
		var = 'PYTHON3'
	)
	conf.find_program(
		'img_to_src',
		var = 'IMG_TO_SRC',
		exts = '.py',
		path_list = os.path.abspath('.')
	)
	
	conf.find_program(
		'find_tile',
		var = 'FIND_TILE',
		exts = '.py',
		path_list = os.path.abspath('.')
	)

def build(bld):
	bld.recurse('emulator')
	
	screen_imgs = sorted(
		glob.glob('images/tiles.png') +
		glob.glob('images/link_sheet.png') + 
		glob.glob('images/fonts_red.png') +
		glob.glob('images/fonts_white.png') +  
		glob.glob('images/HUD_sprites.png') +
		glob.glob('images/loshmee.png') +
		glob.glob('images/dungeon.png')
	)

	bld(
		rule = '${PYTHON} ${FIND_TILE}',
		target = ['screen_tile_index.c', 'screen_tile_index.h']
	)

	bld(
		rule = '${PYTHON} ${IMG_TO_SRC} -o ${TGT[0]} ${SRC} ' + \
			'-f IDX4 -p 0x000000 -v',
		source = screen_imgs,
		target = ['screens_idx4.c', 'screens_idx4.h']
	)

	bld(
		rule = '${PYTHON} ${IMG_TO_SRC} -o ${TGT[0]} ${SRC} ' + \
			'-f IDX4 -p 0x000000 -v',
		source = 'images/title_screen.png',
		target = ['title_screen_idx4.c', 'title_screen_idx4.h']
	)

	bld.program(
		features = 'cxx',
		source = ['project.c', 'screens_idx4.c', 'title_screen_idx4.c', 'screen_tile_index.c'],
		includes = ['build/'],
		use = 'emulator',
		target = 'project'
	)

def run(ctx):
	ctx.exec_command2('./build/project')

###############################################################################

def exec_command2(self, cmd, **kw):
	# Log output while running command.
	kw['stdout'] = None
	kw['stderr'] = None
	ret = self.exec_command(cmd, **kw)
	if ret != 0:
		self.fatal('Command "{}" returned {}'.format(cmd, ret))
setattr(waflib.Context.Context, 'exec_command2', exec_command2)

###############################################################################

def recursive_glob(pattern, directory = '.'):
	for root, dirs, files in os.walk(directory, followlinks = True):
		for f in files:
			if fnmatch.fnmatch(f, pattern):
				yield os.path.join(root, f)
		for d in dirs:
			if fnmatch.fnmatch(d + '/', pattern):
				yield os.path.join(root, d)

def collect_git_ignored_files():
	for gitignore in recursive_glob('.gitignore'):
		with open(gitignore) as f:
			base = os.path.dirname(gitignore)
			
			for pattern in f.readlines():
				pattern = pattern[:-1]
				for f in recursive_glob(pattern, base):
					yield f

###############################################################################

def distclean(ctx):
	for fn in collect_git_ignored_files():
		if os.path.isdir(fn):
			shutil.rmtree(fn)
		else:
			os.remove(fn)

def dist(ctx):
	now = datetime.datetime.now()
	time_stamp = '{:d}-{:02d}-{:02d}-{:02d}-{:02d}-{:02d}'.format(
		now.year,
		now.month,
		now.day,
		now.hour,
		now.minute,
		now.second
	)
	ctx.arch_name = '../{}-{}.zip'.format(APPNAME, time_stamp)
	ctx.algo = 'zip'
	ctx.base_name = APPNAME
	# Also pack git.
	waflib.Node.exclude_regs = waflib.Node.exclude_regs.replace(
'''
**/.git
**/.git/**
**/.gitignore''', '')
	# Ignore waf's stuff.
	waflib.Node.exclude_regs += '\n**/.waf*'
	
###############################################################################
