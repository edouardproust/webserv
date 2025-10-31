#!/usr/bin/env python3

import cgi
import cgitb
import os
import sys
import urllib.parse
from html import escape
from datetime import datetime

cgitb.enable()  # activate debug on CGI

print("Content-Type: text/html\r") # mandatory
print("\r") # empty line to seperate headers and body

method = os.environ.get("REQUEST_METHOD", "UNKNOWN")

print(f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Webserv CGI Universal Test (Python)</title>
</head>
<body>
<h1>Python CGI Universal Test</h1>
<p><strong>Request Method:</strong> {method}</p>
""")

# GET parameters
query_string = os.environ.get("QUERY_STRING", "")
if query_string:
    get_params = urllib.parse.parse_qs(query_string)
    print("<h2>GET Parameters:</h2><pre>")
    for k, v in get_params.items():
        print(f"{escape(k)} = {escape(','.join(v))}")
    print("</pre>")

# POST parameters (application/x-www-form-urlencoded)
if method == "POST":
    try:
        form = cgi.FieldStorage()
        if form:
            print("<h2>POST Parameters:</h2><pre>")
            for key in form.keys():
                value = form.getvalue(key)
                print(f"{escape(key)} = {escape(str(value))}")
            print("</pre>")
    except Exception as e:
        print(f"<p>Error parsing POST: {e}</p>")

# Raw input (PUT, DELETE, or anything else)
if method in ["PUT", "DELETE"] or (method not in ["GET", "POST"]):
    try:
        raw_body = sys.stdin.read()
        if raw_body:
            print(f"<h2>Raw Input ({method}):</h2><pre>{escape(raw_body)}</pre>")
    except Exception as e:
        print(f"<p>Error reading raw input: {e}</p>")

# Headers (HTTP_*)
print("<h2>Request Headers:</h2><pre>")
for k, v in os.environ.items():
    if k.startswith("HTTP_"):
        print(f"{escape(k)} = {escape(v)}")
print("</pre>")

# User agent & server time
user_agent = os.environ.get("HTTP_USER_AGENT", "(unknown)")
print(f"<p>Current server time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>")
print(f"<p>Your user agent: {escape(user_agent)}</p>")

# Optional simple POST form
print("""
<p>Fill a simple form for POST test:</p>
<form action="" method="POST">
<label for="name">Your name:</label><br>
<input type="text" id="name" name="name"><br><br>
<input type="submit" value="Submit">
</form>
""")

print("</body></html>")
