<?php
$input = file_get_contents('php://input');
$size = strlen($input);

echo "Content-type: text/html\r\n\r\n";
echo "<h1>Upload Test</h1>";
echo "<p>Received: " . number_format($size) . " bytes (" . round($size/1024/1024, 2) . " MB)</p>";
echo "<p>First 100 chars: " . htmlspecialchars(substr($input, 0, 100)) . "</p>";
echo "<p>Last 100 chars: " . htmlspecialchars(substr($input, -100)) . "</p>";
?>