#!/usr/bin/env python3
import cgi
import cgitb
import os
import datetime
import html

# Enable CGI error reporting
cgitb.enable()

print("Content-Type: text/html; charset=utf-8")
print()

# Get form data
form = cgi.FieldStorage()

html_template = """<!DOCTYPE html>
<html lang="en">
	<head>
	<meta charset="UTF-8">
		<title>Python CGI Tester</title>
	</head>

	<body>
		<h1>Python CGI Tester</h1>

		<p><strong>Request Method:</strong> {request_method}</p>
		<p>Current server time: {current_time}</p>

		{get_section}
		{post_section}
		{raw_body_section}

		<h2>Environment variables:</h2>
		<h3>Request headers</h3>
		<pre>{env_headers_section}</pre>
		<h3>Others</h3>
		<pre>{env_others_section}</pre>

	</body>
</html>"""

# Helper functions
def escape(text):
	return html.escape(str(text))

def get_header_env_vars():
	headers = []
	for key, value in os.environ.items():
		if key.startswith('HTTP_'):
			headers.append(f"{escape(key)} = {escape(value)}")
	return "\n".join(headers)


def get_other_env_vars():
	others = []
	for key, value in os.environ.items():
		if not key.startswith('HTTP_'):
			others.append(f"{escape(key)} = {escape(value)}")
	return "\n".join(others)

def get_form_data(form, method):
	if not form.keys():
		return ""

	data = []
	for key in form.keys():
		field = form[key]
		if key not in get_params:
			if isinstance(field, list):
				values = [escape(item.value) for item in field]
				value = ", ".join(values)
			else:
				value = escape(field.value)
			data.append(f"{escape(key)} = {value}")
	return "\n".join(data)

# Build sections
request_method = escape(os.environ.get('REQUEST_METHOD', 'GET'))
current_time = escape(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

# GET section
get_params = os.environ.get('QUERY_STRING', '')
if get_params:
	get_data = []
	for param in get_params.split('&'):
		if '=' in param:
			key, value = param.split('=', 1)
			get_data.append(f"{escape(key)} = {escape(value)}")
		else:
			get_data.append(f"{escape(param)} = ")
	get_section = f"""
		<h2>GET Parameters:</h2>
		<pre>{"\n".join(get_data)}</pre>
	"""
else:
    get_section = ""

# POST section
if request_method == 'POST' and form.keys():
	post_data = get_form_data(form, 'POST')
	post_section = f"""
		<h2>POST Parameters:</h2>
		<pre>{post_data}</pre>
	"""
else:
	post_section = """
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
	"""

# Raw body section (for other methods like PUT)
raw_body_section = ""
if request_method not in ['GET', 'POST'] or (request_method == 'POST' and not form.keys()):
	try:
		raw_body = sys.stdin.read()
		if raw_body and not form.keys():
			raw_body_section = f"""
				<h2>Raw Input ({escape(request_method)}):</h2>
				<pre>{escape(raw_body)}</pre>
			"""
		elif not get_params and not form.keys():
			raw_body_section = "<p>No GET or POST parameters received.</p>"
	except:
		pass

# Environment variables sections
env_headers_section = get_header_env_vars()
env_others_section = get_other_env_vars()

# Output the final HTML
print(html_template.format(
	request_method=request_method,
	current_time=current_time,
	get_section=get_section,
	post_section=post_section,
	raw_body_section=raw_body_section,
	env_headers_section=env_headers_section,
	env_others_section=env_others_section
))