--TEST--
epeg_decode_bounds_set() function
--SKIPIF--
<?php
include 'skipif.inc';
if (!function_exists('epeg_decode_bounds_set')) {
    die('skip epeg_decode_bounds_set() is not available');
}
?>
--FILE--
<?php
echo 'OK'; // no test case for this function yet
?>
--EXPECT--
OK