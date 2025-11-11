<?php
echo "Content-Type: text/plain\r\n\r\n";
echo "SCRIPT_NAME: " . $_SERVER['SCRIPT_NAME'] . "\n";
echo "PATH_INFO: " . ($_SERVER['PATH_INFO'] ?? 'NONE') . "\n";
echo "QUERY_STRING: " . ($_SERVER['QUERY_STRING'] ?? 'NONE') . "\n";

// If PATH_INFO looks like a file, try to read it
$path_info = $_SERVER['PATH_INFO'] ?? '';
if ($path_info && file_exists('..' . $path_info)) {
    echo "FILE CONTENT: " . file_get_contents('..' . $path_info) . "\n";
} else {
    echo "FILE: Not found or no PATH_INFO\n";
}
?>