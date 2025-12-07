<?php

$wait = 300; // seconds

sleep($wait);

echo "<h1>Oops...</h1>";
echo "<h2>The timeout process was not killed correctly!</h2>";
echo "<p>";
echo "<div>If you can read this, it means that the timeout security for CGI is not working on this web server.</div>";
echo "<div>Or you need to increase the sleep time (" . $wait . " seconds) in <code>" . __FILE__ . "</code></div>";
echo "</p>";
?>
