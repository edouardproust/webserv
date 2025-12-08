#!/usr/bin/env python3
import cgi
import cgitb
import os
import sys
import datetime
import html

cgitb.enable()

print("Content-Type: text/html; charset=utf-8")
print()

def escape(v):
	return html.escape(str(v))

request_method = os.environ.get("REQUEST_METHOD", "GET")
current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# ---------- GET PARAMETERS ----------
get_params = {}
query_string = os.environ.get("QUERY_STRING", "")

if query_string:
	for pair in query_string.split("&"):
		if "=" in pair:
			k, v = pair.split("=", 1)
		else:
			k, v = pair, ""
		get_params.setdefault(k, []).append(v)

get_section = ""
if get_params:
	lines = [f"{escape(k)} = {escape(', '.join(v))}" for k, v in get_params.items()]
	get_section = f"""
<h2>GET Parameters</h2>
<pre>{"\n".join(lines)}</pre>
"""

# ---------- POST PARAMETERS ----------
post_params = {}

if request_method == "POST":
	environ = os.environ.copy()
	environ["QUERY_STRING"] = ""   # <-- IMPORTANT
	form = cgi.FieldStorage(fp=sys.stdin, environ=environ, keep_blank_values=True)

	for key in form.keys():
		field = form[key]
		if isinstance(field, list):
			values = [item.value for item in field]
		else:
			values = [field.value]
		post_params[key] = values

post_section = ""
if post_params:
	lines = [f"{escape(k)} = {escape(', '.join(v))}" for k, v in post_params.items()]
	post_section = f"""
<h2>POST Parameters</h2>
<pre>{"\n".join(lines)}</pre>
"""
else:
	post_section = """
<h2>POST test form</h2>
<form method="POST">
<input type="text" name="name" placeholder="Name"><br>
<input type="text" name="age" placeholder="Age"><br>
<input type="submit" value="Submit">
</form>
"""

# ---------- ENV ----------
def env_block(prefix):
	return "\n".join(
		f"{escape(k)} = {escape(v)}"
		for k, v in os.environ.items()
		if (prefix == "HTTP_" and k.startswith("HTTP_"))
		or (prefix == "" and not k.startswith("HTTP_"))
	)

html_page = f"""<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>CGI Test</title></head>
<body>

<h1>Python CGI Tester</h1>

<p><b>Method:</b> {escape(request_method)}</p>
<p><b>Time:</b> {escape(current_time)}</p>

{get_section}
{post_section}

<h2>Environment variables</h2>

<h3>Request headers</h3>
<pre>{env_block("HTTP_")}</pre>

<h3>Others</h3>
<pre>{env_block("")}</pre>

</body>
</html>
"""

print(html_page)
