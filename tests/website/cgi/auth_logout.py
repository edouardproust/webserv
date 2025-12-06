#!/usr/bin/env python3

def main():
    print("Status: 200 OK")
    print("Content-Type: text/html")
    print("X-Webserv-Destroy-Session: true")
    print()
    print("""
    <html>
    <body>
        <h1>Logged Out</h1>
        <p>You have been successfully logged out.</p>
        <a href='/cgi/auth_login.py'>Login again</a><br>
    </body>
    </html>
    """)

if __name__ == "__main__":
    main()