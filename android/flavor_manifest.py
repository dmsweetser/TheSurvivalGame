#!/usr/bin/env python3
"""Generate a flavor manifest from the shared AndroidManifest.xml.

Usage: flavor_manifest.py <src> <dst> <flavor> <versionCode> <versionName>

flavor "itch": stamps the version and adds INTERNET (the launch-time
    update NOTICE fetches a version file, then opens the game's page
    in the browser — the app never downloads or installs APKs itself;
    REQUEST_INSTALL_PACKAGES + in-app install is what got 1.1 flagged
    by Play Protect, see src-itch Updater.java).
flavor "play": stamps the version only (permissions stay at zero).

Both build scripts stamp versions from android/VERSION so the two
flavors can never drift apart in version identity.
"""
import re
import sys

src, dst, flavor, code, name = sys.argv[1:6]
xml = open(src).read()

xml, n = re.subn(r'android:versionCode="\d+"', f'android:versionCode="{code}"', xml)
assert n == 1, "versionCode attribute not found"
xml, n = re.subn(r'android:versionName="[^"]*"', f'android:versionName="{name}"', xml)
assert n == 1, "versionName attribute not found"

if flavor == "itch":
    perms = (
        '    <uses-sdk android:minSdkVersion="24" android:targetSdkVersion="34" />\n'
        '\n'
        '    <!-- itch flavor only: INTERNET, for the launch-time update\n'
        '         NOTICE (fetch a version file, open the game page in the\n'
        '         browser). Deliberately NO REQUEST_INSTALL_PACKAGES and\n'
        '         no install machinery — that is what Play Protect flags.\n'
        '         The Play build (build_aab.sh) stays at zero permissions. -->\n'
        '    <uses-permission android:name="android.permission.INTERNET" />\n'
    )
    xml, n = re.subn(
        r'    <uses-sdk android:minSdkVersion="24" android:targetSdkVersion="34" />\n',
        perms, xml)
    assert n == 1, "uses-sdk line not found"
elif flavor != "play":
    sys.exit(f"unknown flavor: {flavor}")

open(dst, "w").write(xml)
print(f"manifest -> {dst} (flavor={flavor}, versionCode={code}, versionName={name})")
