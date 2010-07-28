<?php
/**
 * php_epeg/examples
 * create thumbnail by epeg_thumbnail_create() function
 */

require dirname(__FILE__) . DIRECTORY_SEPARATOR . 'common.inc.php';

/**
 * Read from pathname
 */
var_dump(epeg_thumbnail_create($sampleimg, 'thumb.jpg', 160, 160, 80));

/**
 * Read from www
 */
//var_dump(epeg_thumbnail_create('http://example.com/example.jpg', 'thumb-from-web.jpg', 160, 160));
