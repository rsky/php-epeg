<?php
if (!extension_loaded('epeg') && ini_get('enable_dl')) {
    $dlext = !strncasecmp(PHP_OS, 'WIN', 3) ? '.dll' : '.so';
    @dl("epeg$dlext");
}
if (!extension_loaded('epeg')) {
    die('skip epeg extension is not loaded');
}
