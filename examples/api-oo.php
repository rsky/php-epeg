<?php
/**
 * php_epeg/examples
 * create thumbnail by OO API
 */

require dirname(__FILE__) . DIRECTORY_SEPARATOR . 'common.inc.php';

$epeg = new Epeg($sampleimg);

$size = $epeg->getSize();
print_r($size);

$epeg->setDecodeSize(160, 120);
$epeg->setQuality(80);

var_dump($epeg->encode('thumb-by-oo.jpg'));
