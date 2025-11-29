#!/usr/bin/env python3
import os
import cgi
import cgitb
cgitb.enable()

def main():
    form = cgi.FieldStorage()
    username = form.getvalue('username')
    
    if username:
        # Successful login - create session
        print("Status: 200 OK")
        print("Content-Type: text/html")
        print(f"X-Webserv-Create-Session: {username}")
        print()
        print(f"""
        <html>
        <body>
            <h1>Login Successful!</h1>
            <p>Welcome {username}!</p>
            <a href='/cgi/profile.py'>View Profile</a><br>
            <a href='/cgi/auth_logout.py'>Logout</a>
        </body>
        </html>
        """)
    else:
        # Show login form
        print("Content-Type: text/html")
        print()
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