#include "cgi/CgiContext.hpp"

CgiContext::CgiContext(pid_t p, int cfd, int out, int err, bool closeConn, Request const* req)
: pid(p)
, clientFd(cfd)
, stdoutPipe(out)
, stderrPipe(err)
, startTime(time(NULL))
, connectionClose(closeConn)
, originalRequest(req)
{}