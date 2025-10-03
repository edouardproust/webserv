<?php
  header("Content-Type: text/html");
?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Webserv CGI Test (PHP)</title>
</head>
<body>
  <h1>PHP CGI Test</h1>
  <p>If you see this page, CGI execution works correctly.</p>
  <p>Current server time: <?php echo date("Y-m-d H:i:s"); ?></p>
  <p>Your user agent: <?php echo $_SERVER['HTTP_USER_AGENT']; ?></p>
</body>
</html>
