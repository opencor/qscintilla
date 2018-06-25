# This script handles the creation of the PEP 376 .dist-info directory for a
# package.
#
# Copyright (c) 2018 Riverbank Computing Limited <info@riverbankcomputing.com>
#
# This script is distributed under the terms of the GNU General Public License
# v3 as published by the Free Software Foundation.
#
# This script is supplied WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


import base64
import hashlib
import os
import shutil
import sys


def error(message):
    """ Display an error message and terminate. """

    sys.stderr.write(message + '\n')
    sys.exit(1)


# Parse the command line.
if len(sys.argv) != 3:
    error("usage: {0} dist-info installed".format(sys.argv[0]))

distinfo_dir = sys.argv[1]
installed_fn = sys.argv[2]

# Read the list of installed files.
installed_f = open(installed_fn)
installed = installed_f.read().strip().split('\n')
installed_f.close()

# Remove any existing dist-info directory and create an empty one.
if os.path.exists(distinfo_dir):
    try:
        shutil.rmtree(distinfo_dir)
    except:
        error("unable to delete existing {0}".format(distinfo_dir))

try:
    os.mkdir(distinfo_dir)
except:
    error("unable to create {0}".format(distinfo_dir))

# Create the INSTALLER file.  We pretend that pip was the installer.
installer_fn = os.path.join(distinfo_dir, 'INSTALLER')
installer_f = open(installer_fn, 'w')
installer_f.write('pip\n')
installer_f.close()

installed.append(installer_fn)

# Create the METADATA file.
METADATA = '''Metadata-Version: 1.1
Name: {0}
Version: {1}
'''

distinfo_path, distinfo_base = os.path.split(distinfo_dir)
pkg_name, version = os.path.splitext(distinfo_base)[0].split('-')

metadata_fn = os.path.join(distinfo_dir, 'METADATA')
metadata_f = open(metadata_fn, 'w')
metadata_f.write(METADATA.format(pkg_name, version))
metadata_f.close()

installed.append(metadata_fn)

# Create the RECORD file.
record_fn = os.path.join(distinfo_dir, 'RECORD')
record_f = open(record_fn, 'w')

for name in installed:
    native_name = name.replace('/', os.sep)
    if os.path.isdir(native_name):
        all_fns = []

        for root, dirs, files in os.walk(native_name):
            for f in files:
                all_fns.append(os.path.join(root, f).replace(os.sep, '/'))

            if '__pycache__' in dirs:
                dirs.remove('__pycache__')
    else:
        all_fns = [name]

    for fn in all_fns:
        if fn.startswith(distinfo_path):
            fn_name = fn[len(distinfo_path) + 1:].replace('\\', '/')
        elif fn.startswith(sys.prefix):
            fn_name = os.path.relpath(fn, distinfo_path).replace('\\', '/')
        else:
            fn_name = fn

        fn_f = open(fn, 'rb')
        data = fn_f.read()
        fn_f.close()

        digest = base64.urlsafe_b64encode(
                hashlib.sha256(data).digest()).rstrip(b'=').decode('ascii')

        record_f.write(
                '{0},sha256={1},{2}\n'.format(fn_name, digest, len(data)))

record_f.write('{0}/RECORD,,\n'.format(distinfo_base))

record_f.close()
