<?php 
/**
 * php_epeg/examples
 * show constants, functions and methods provides by mecab extension
 */

require dirname(__FILE__) . DIRECTORY_SEPARATOR . 'common.inc.php';

$linebreak = PHP_EOL . PHP_EOL;

border();

echo 'Constants:', $linebreak;
$constants = get_defined_constants(true);
if (function_exists('unicode_semantics') && unicode_semantics()) {
    eval('print_r($constants[(binary)"epeg"]);');
} else {
    print_r($constants['epeg']);
}

border();

echo 'Functions:', $linebreak;
$functions = get_extension_funcs('epeg');
print_r($functions);

border();

if (class_exists('Epeg')) {
    echo 'Methods:', $linebreak;
    $methods = array();
    $methods['Epeg'] = get_class_methods('Epeg');
    print_r($methods);
    border();
}
