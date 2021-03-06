#!/usr/bin/env python

# Copyright (c) 2018-2019, NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

import os
import sys
from xml.dom.minidom import parse

top_dir =  os.environ['TOP']
manifest_file = top_dir + '/.repo/manifests/default.xml'
xml_tree = parse(manifest_file)
collection = xml_tree.documentElement
projects = collection.getElementsByTagName("default")

for prj in projects:
    if prj.getAttribute("remote") == "github":
        ver = prj.getAttribute("revision").split('-')[1].split('.')
        print "%s.%s" % (ver[0].zfill(2), ver[1].zfill(2))
        break
