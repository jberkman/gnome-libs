#!/bin/sh
cat > /tmp/.$$.html
lynx -dump /tmp/.$$.html
rm -f /tmp/.$$.html
