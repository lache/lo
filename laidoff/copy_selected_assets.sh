#!/bin/bash
cat clientlog.txt | grep "loaded to memory" | cut -d" " -f2 | cut -d'/' -f5- | cpio -pvdmB selected-assets-br

