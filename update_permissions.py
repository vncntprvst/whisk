#!/usr/bin/env python

import os
import stat
import distutils.sysconfig

def update_permissions():
    print("Updating permissions of files in the 'bin' directory...")

    # Locate the 'bin' directory in the site-packages
    site_packages_dir = distutils.sysconfig.get_python_lib()
    bin_dir = os.path.join(site_packages_dir, 'whisk', 'bin')

    if not os.path.exists(bin_dir):
        print("Error: 'bin' directory not found. Please ensure the package is installed correctly.")
        return

    # Update the permissions of each file in the 'bin' directory
    for filename in os.listdir(bin_dir):
        file_path = os.path.join(bin_dir, filename)

        if os.path.isfile(file_path):
            # if on Windows skip files that are not executable
            if os.name == 'nt' and not filename.endswith('.exe'):
                continue
            elif os.name == 'posix' and (filename.endswith('.dll') or filename.endswith('.exe')):
                continue
            # For current user: read, write, execute; for group and others: read only
            os.chmod(file_path, stat.S_IRWXU | stat.S_IRGRP | stat.S_IROTH)
            # For all users: read, write, execute
            # os.chmod(file_path, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
            print(f"Updated permissions for {filename}")

    print("Permissions update complete.")

if __name__ == "__main__":
    update_permissions()
