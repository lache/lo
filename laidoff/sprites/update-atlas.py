import os
import json
import glob
import re
import xml.etree.ElementTree as ET
from shutil import copyfile

all_sprites = []

r = re.compile('^atlas\d\d$')

for i, g in enumerate(glob.glob('atlas*')):

	if os.path.isdir(g) and r.match(g):
		
		tree = ET.parse(os.path.join(g, 'sprites.xml'))
		root = tree.getroot()
		
		total_sprites_per_atlas = len(root)
		
		sprites = []
	
		for f in root:
			sprites.append((f.get('x'), f.get('y'), f.get('width'), f.get('height'), f.get('name')))
		
		all_sprites.append(sprites)
		
		src_png_path = os.path.join(g, 'sprites.png')
		dst_png_path = os.path.join('..', 'assets', 'tex', g + '.png')
		print('Copying %s to %s...' % (src_png_path, dst_png_path))
		copyfile(src_png_path, dst_png_path)

sprite_data_h_path = os.path.join('..', 'src', 'sprite_data.h')
		
with open(sprite_data_h_path, 'w') as f:

	f.write("// Automatically generated code. Do not edit.\n")
	f.write("#pragma once\n")
	f.write("#include \"sprite.h\"\n")
	f.write("static LWSPRITE SPRITE_DATA[][%d] = {\n" % total_sprites_per_atlas)

	for sprites in all_sprites:
		f.write("	{\n")
		for s in sprites:
			f.write("		{ %s, %s, %s, %s }, // %s\n" % s)
		f.write("	},\n")
		
	f.write("};\n")

print('Writing %s...' % sprite_data_h_path)
