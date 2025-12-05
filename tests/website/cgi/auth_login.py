#!/usr/bin/env python3
import os
import cgi

def main():
    request_method = os.environ.get('REQUEST_METHOD', 'GET')
    form = cgi.FieldStorage()
    username = form.getvalue('username')
    
    if request_method == 'POST' and username:
        # POST: Create session and REDIRECT to profile
        print("Status: 302 Found")
        print(f"X-Webserv-Create-Session: {username}")
        print("Location: /cgi/profile.py")  # Redirect to GET page
        print("Content-Type: text/html")
        print()
        return
    
    # GET: Show login form (or already logged in message)
    print("Content-Type: text/html")
    print()
    
    # Optional: Check if already logged in
    cookies = os.environ.get('HTTP_COOKIE', '')
    if 'session_id=' in cookies:
        print("""
        <html>
        <body>
            <h1>Already Logged In</h1>
            <p><a href='/cgi/profile.py'>Go to Profile</a></p>
            <p><a href='/cgi/auth_logout.py'>Logout</a></p>
        </body>
        </html>
        """)
    else:
        print("""
        <html>
        <body>
            <h1>Login</h1>
            <form method='POST'>
                Username: <input type='text' name='username' required><br>
                <input type='submit' value='Login'>
            </form>
        </body>
        </html>
        """)

if __name__ == "__main__":
    main()