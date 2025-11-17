<!DOCTYPE html>
<html lang="en">
	<head>
	<meta charset="UTF-8">
		<title>PHP CGI Tester</title>
	</head>

	<body>
		<h1>PHP CGI Tester</h1>

		<p><strong>Request Method:</strong> <?= htmlspecialchars($_SERVER['REQUEST_METHOD']); ?></p>
		<p>Current server time: <?= date("Y-m-d H:i:s"); ?></p>

		<?php if (!empty($_GET)): ?>
			<h2>GET Parameters:</h2>
			<pre><?php
				foreach ($_GET as $key => $value) {
					echo htmlspecialchars($key) . " = " . htmlspecialchars($value) . "\n";
				}
			?></pre>
		<?php endif; ?>

		<?php if (!empty($_POST)): ?>
			<h2>POST Parameters:</h2>
			<pre><?php
				foreach ($_POST as $key => $value) {
					echo htmlspecialchars($key) . " = " . htmlspecialchars($value) . "\n";
				}
			?></pre>
		<?php else: ?>
			<p><h2>POST test form:</h2></p>
				<form action="" method="POST">
					<table>
						<tr>
							<td><label for="name">Your name:</label></td>
							<td><input type="text" id="name" name="name"></td>
						</tr><tr>
							<td><label for="age">Your age:</label></td>
							<td><input type="text" id="age" name="age"></td>
						</tr><tr>
							<td></td>
							<td><input type="submit" value="Submit"></td>
						</tr>
					</table>
				</form>
		<?php endif; ?>

		<h2>Environment variables</h2>

		<h3>Request headers</h3>
		<pre><?php foreach ($_SERVER as $key => $value) {
			if (strpos($key, 'HTTP_') === 0) {
				echo htmlspecialchars($key) . " = " . htmlspecialchars($value) . "\n";
			}
		} ?></pre>

		<h3>Others</h3>
		<pre><?php foreach ($_SERVER as $key => $value) {
			if (strpos($key, 'HTTP_') !== 0) {
				echo htmlspecialchars($key) . " = " . htmlspecialchars($value) . "\n";
			}
		} ?></pre>

	</body>
</html>
