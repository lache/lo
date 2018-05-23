import glob
import posixpath
import hashlib
import binascii
import subprocess
import sys
import os

def hash_bytestr_iter(bytesiter, hasher, ashexstr=False):
    for block in bytesiter:
        hasher.update(block)
    return (hasher.hexdigest() if ashexstr else hasher.digest())

def file_as_blockiter(afile, blocksize=65536):
    with afile:
        block = afile.read(blocksize)
        while len(block) > 0:
            yield block
            block = afile.read(blocksize)

def update_list():
	print('Update list file (recalculating hashes)...')
	filelist = []

	for filename in glob.iglob('assets/*/**/*.*', recursive=True):
		p = posixpath.join(*filename.split('\\'))
		
		h = hash_bytestr_iter(file_as_blockiter(open(filename, 'rb')), hashlib.md5())
		
		h_str = binascii.hexlify(h).decode()
		
		line = '%s\t%s' % (p, h_str)
		
		print(line)
		filelist.append(line)

	with open('assets/list.txt', 'w') as f:
		f.write('\n'.join(filelist))

def sync_s3():
	print('Syncing s3...')
	subprocess.call(['aws' if os.name == 'nt' else 'aws', 's3', 'sync', 'assets', 's3://sky.popsongremix.com/laidoff/assets', '--acl', 'public-read'])
		
if __name__ == '__main__':
	if sys.version_info[0] < 3:
		raise Exception("Python 3 or a more recent version is required.")
	update_list()
	sync_s3()
	