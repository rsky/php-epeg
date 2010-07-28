<?php
/**
 * php_epeg/examples
 * multiple output
 */

require dirname(__FILE__) . DIRECTORY_SEPARATOR . 'common.inc.php';

if ($argc != 2 || $argv[1] != 'make') {
    echo 'Usage:', PHP_EOL;
    echo '  php ', $argv[0], ' make > thumb2.jpg 2> thumb3.jpg', PHP_EOL;
    echo '  md5 thumb1.jpg thumb2.jpg thumb3.jpg', PHP_EOL;
} else {
    // write to file
    epeg_thumbnail_create($sampleimg, 'thumb1.jpg', 160, 160, 80);
    // print
    echo epeg_thumbnail_create($sampleimg, '', 160, 160, 80);
    // write to STDERR
    epeg_thumbnail_create($sampleimg, 'php://stderr', 160, 160, 80);
}
