<?php
/**
 * php_epeg/examples
 * create thumbnail by function API
 */

require dirname(__FILE__) . DIRECTORY_SEPARATOR . 'common.inc.php';

$im = epeg_open($sampleimg);

$size = epeg_size_get($im);
print_r($size);

epeg_decode_size_set($im, 160, 120);
epeg_quality_set($im, 80);

var_dump(epeg_encode($im, 'thumb-by-func.jpg'));
epeg_close($im);
