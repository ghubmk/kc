--TEST--
mapi_table_setcolumns() server tests
--SKIPIF--
<?php if (!extension_loaded("mapi") || !getenv("KOPANO_SOCKET")) print "skip"; ?>
--FILE--
<?php
require_once(__DIR__.'/helpers.php');

$session = getMapiSession();
$store = getDefaultStore($session);
$root = mapi_msgstore_openentry($store);
$table = mapi_folder_gethierarchytable($root);
var_dump(mapi_table_setcolumns($table,  array(PR_DISPLAY_NAME, PR_ENTRYID)));
--EXPECT--
bool(true)
