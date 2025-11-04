<!DOCTYPE html>
<html lang="en">
	<head>
	<meta charset="UTF-8">
		<title>Webserv CGI Test (PHP)</title>
	</head>

	<body>
		<h1>PHP CGI Universal Test</h1>

		<p><strong>Request Method:</strong> <?= htmlspecialchars($_SERVER['REQUEST_METHOD']); ?></p>

		<?php if (!empty($_GET)): ?>
			<h2>GET Parameters:</h2>
			<pre><?php
				foreach ($_GET as $key => $value) {
					echo htmlspecialchars($key) . " = " . htmlspecialchars($value) . "\n";
				}
			?></pre>
		<?php endif ?>

		<?php if (!empty($_POST)): ?>
			<h2>POST Parameters:</h2>
			<pre><?php
				foreach ($_POST as $key => $value) {
					echo htmlspecialchars($key) . " = " . htmlspecialchars($value) . "\n";
				}
			?></pre>

		<?php else:
			$raw_body = file_get_contents('php://input');
			if (!empty($raw_body)): ?>
				<h2>Raw Input (<?php echo $_SERVER['REQUEST_METHOD']; ?>):</h2>
				<pre><?= htmlspecialchars($raw_body); ?></pre>
			<?php else: ?>
				<p>No GET or POST parameters received.</p>
			<?php endif; ?>

		<?php endif; ?>

		<h2>Request Headers:</h2>
		<pre><?php
		foreach ($_SERVER as $key => $value) {
			if (strpos($key, 'HTTP_') === 0) {
				echo htmlspecialchars($key) . " = " . htmlspecialchars($value) . "\n";
			}
		}
		?></pre>

		<p>Current server time: <?= date("Y-m-d H:i:s"); ?></p>
		<p>Your user agent: <?= isset($_SERVER['HTTP_USER_AGENT']) ?
				htmlspecialchars($_SERVER['HTTP_USER_AGENT']) : '(unknown)'; ?>
		</p>

		<p>Fill a simple form for POST test:</p>
		<form action="" method="POST">
			<label for="name">Your name:</label><br>
			<input type="text" id="name" name="name"><br><br>
			<input type="submit" value="Submit">
		</form>

	</body>
</html>
