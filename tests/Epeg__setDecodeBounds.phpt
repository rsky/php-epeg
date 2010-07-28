--TEST--
Epeg::setDecodeBounds() method
--SKIPIF--
<?php
include 'skipif_oo.inc';
if (!method_exists('Epeg', 'setDecodeBounds')) {
    die('skip Epeg::setDecodeBounds() is not available');
}
?>
--FILE--
<?php
echo 'OK'; // no test case for this function yet
?>
--EXPECT--
OK