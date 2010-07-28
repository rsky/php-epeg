<?php
/**
 * php_epeg/examples
 * common configuration file
 */

if (!extension_loaded('epeg')) {
    $_module_suffix = (PHP_SHLIB_SUFFIX == 'dylib') ? 'so' : PHP_SHLIB_SUFFIX;
    dl('epeg.' . $_module_suffix) || die('skip');
}

$sampleimg = 'sample.jpg';
$_il = __LINE__ - 1;
if (!file_exists($sampleimg)) {
    printf('%s not exists.%s', $sampleimg, PHP_EOL);
    printf('please edit line %d in %s.%s', $_il, __FILE__, PHP_EOL);
    exit(1);
}

function border()
{
    static $border = NULL;
    if ($border === NULL) {
        $border = PHP_EOL . str_repeat('-', 72) . PHP_EOL . PHP_EOL;
    }
    echo $border;
}

function catcher($errno, $errstr, $errfile, $errline, $errcontext)
{
    border();
    printf('%s:%d: %s%s', $errfile, $errline, $errstr, PHP_EOL);
    //print_r($errcontext);
    die($errno);
}

set_error_handler('catcher');
