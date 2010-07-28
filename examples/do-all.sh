#!/bin/sh

if [ -z "$PHP_BIN" ]; then
	PHP_BIN=php
fi

if [ -z "$UNICODE" ]; then
	OPTIONS=""
else
	OPTIONS="-dunicode.semantics=1"
fi

echo "--"
echo "api-func.php:"
"$PHP_BIN" $OPTIONS api-func.php

echo "--"
echo "api-oo.php:"
"$PHP_BIN" $OPTIONS api-oo.php

echo "--"
echo "mkthumb.php:"
"$PHP_BIN" $OPTIONS mkthumb.php

echo "--"
echo "multiple.php make > thumb2.jpg 2> thumb3.jpg:"
"$PHP_BIN" $OPTIONS multiple.php make > thumb2.jpg 2> thumb3.jpg

echo "--"
echo "provides.php:"
"$PHP_BIN" $OPTIONS provides.php

echo "--"
echo "rmmarker.php:"
"$PHP_BIN" $OPTIONS rmmarker.php

echo "--"
wc -c *.jpg
md5 *.jpg
rm no-marker.jpg thumb*.jpg
