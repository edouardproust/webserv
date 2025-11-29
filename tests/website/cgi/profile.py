#!/usr/bin/env python3
import os

def main():
    # Get cookies from environment FIRST
    cookies = os.environ.get('HTTP_COOKIE', '')
    
    # Extract session_id from cookies
    session_id = None
    if cookies:
        for cookie in cookies.split(';'):
            cookie = cookie.strip()
            if cookie.startswith('session_id='):
                session_id = cookie.split('session_id=')[1].strip()
                break

    # NOW output headers and content
    print("Content-Type: text/html")
    print()
    
    # Start HTML output
    print("""
    <html>
    <head>
        <title>Profile Debug</title>
        <style>
            .debug { background: #f0f0f0; padding: 10px; margin: 10px 0; border-left: 4px solid #007cba; }
            .success { background: #d4edda; padding: 15px; border-radius: 5px; }
            .error { background: #f8d7da; padding: 15px; border-radius: 5px; }
        </style>
    </head>
    <body>
        <h1>Profile Page - Debug</h1>
    """)
    
    # Debug section
    print("""
        <div class="debug">
            <h3>Environment Variables:</h3>
            <p><strong>HTTP_COOKIE:</strong> '{}'</p>
            <p><strong>All env vars with 'COOKIE':</strong></p>
            <ul>
    """.format(cookies))
    
    # List all environment variables that contain "COOKIE"
    cookie_env_vars = []
    for key, value in os.environ.items():
        if 'COOKIE' in key.upper():
            cookie_env_vars.append((key, value))
            print("<li><strong>{}:</strong> '{}'</li>".format(key, value))
    
    if not cookie_env_vars:
        print("<li>No environment variables containing 'COOKIE' found!</li>")
    
    print("""
            </ul>
        </div>
        
        <div class="debug">
            <h3>Cookie Parsing:</h3>
            <p><strong>Raw cookies string:</strong> '{}'</p>
            <p><strong>Extracted session_id:</strong> '{}'</p>
        </div>
    """.format(cookies, session_id if session_id else "None"))

    if session_id:
        # User is logged in
        print("""
        <div class="success">
            <h3>Successfully Logged In!</h3>
            <p>Session ID: {}</p>
        </div>
        """.format(session_id))
    else:
        # Not logged in
        print("""
        <div class="error">
            <h3>Access Denied</h3>
            <p>No valid session ID found.</p>
            <p>Possible reasons:</p>
            <ul>
                <li>HTTP_COOKIE environment variable is empty</li>
                <li>session_id not found in cookies</li>
                <li>Cookie parsing failed</li>
            </ul>
        </div>
        """)

    print("""
        <hr>
        <p><a href='/cgi/auth_login.py'>Back to Login</a></p>
        <p><a href='/cgi/auth_logout.py'>Logout</a></p>
    </body>
    </html>
    """)

if __name__ == "__main__":
    main()