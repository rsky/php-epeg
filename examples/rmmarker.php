<?php
/**
 * php_epeg/examples
 * do not resize, only remove marker
 */

require dirname(__FILE__) . DIRECTORY_SEPARATOR . 'common.inc.php';

var_dump(epeg_thumbnail_create($sampleimg, 'no-marker.jpg', 30000, 30000));
