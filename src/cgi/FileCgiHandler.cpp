#include "cgi/FileCgiHandler.hpp"

std::string const	FileCgiHandler::_TMP_INPUT_FILE = "tmp/in";
std::string const	FileCgiHandler::_TMP_OUTPUT_FILE = "tmp/out";

FileCgiHandler::FileCgiHandler() {
	_initPipe(_stderrPipe);
}

FileCgiHandler::~FileCgiHandler() {
    _closePipe(_stderrPipe);
}

Response FileCgiHandler::execute(Request const& req, LocationBlock const* loc, std::string const& scriptName, HostPortPair const& listeningOn) {

	_prepareIo(req.getMethod(), req.getBody());
	return Response(); //TODO
	(void)req, (void)loc, (void)scriptName, (void)listeningOn;
}

void	FileCgiHandler::_prepareIo(std::string const& method, std::string const& reqBody)
{
	// tmp files
    _createEmptyTempFile(_TMP_OUTPUT_FILE, false);
	if (method == "POST" || method == "PUT") {
        _writeBodyToTempFile(reqBody);
    }
	// stderr (pipe)
	if (pipe(_stderrPipe) == -1)
        throw ExecException("stderr pipe() failed");
    _setNonBlocking(_stderrPipe[0]);
}

int	FileCgiHandler::_createEmptyTempFile(std::string const& path, bool keepFd)
{
	// Create directory if it does not exist
	// (Muted because `mkdir` function not allowed by subject)
	//std::string dir = path.substr(0, path.find_last_of('/'));
	//if (!utils::isAccessibleDirectory(dir))
	//	mkdir(dir.c_str(), 0755);

	// Create file in the directory
	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd == -1) {
		throw ExecException("Cannot create temp file file");
	}
	if (!keepFd) {
		close(fd);
		return -1;
	}
	return fd;
}

/**
 * Writes the request body to the temporary input file
 * Creates the file if it doesn't exist, overwrites if it exists
 */
void	FileCgiHandler::_writeBodyToTempFile(std::string const& reqBody)
{
	int fd = _createEmptyTempFile(_TMP_INPUT_FILE, true);

    // Write the full body into
    size_t totalWritten = 0;
    size_t bodySize = reqBody.size();
    const char* bodyData = reqBody.c_str();

    while (totalWritten < bodySize) {
        ssize_t n = write(fd, bodyData + totalWritten, bodySize - totalWritten);
        if (n > 0) {
            totalWritten += n;
        } else if (n == -1) {
            close(fd);
            throw ExecException("Write to temp file failed: " + std::string(strerror(errno)));
        }
    }

    close(fd);
    Log::dev("event", "Written " + utils::str(totalWritten) + " bytes to temp file: " + _TMP_INPUT_FILE);
}
