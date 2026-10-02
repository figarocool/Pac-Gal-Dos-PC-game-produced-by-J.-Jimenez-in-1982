#!/bin/sh
set -eu
exec env LD_LIBRARY_PATH=/tmp/pacgal-vita3k/squashfs-root/usr/lib \
 XDG_CACHE_HOME=/tmp/pacgal-emutest/cache XDG_CONFIG_HOME=/tmp/pacgal-emutest/config XDG_DATA_HOME=/tmp/pacgal-emutest/data \
 /tmp/pacgal-vita3k/squashfs-root/usr/bin/Vita3K \
 --config-location /tmp/pacgal-emutest/config.yml --load-config --keep-config "$@"
