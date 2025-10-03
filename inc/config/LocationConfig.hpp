class LocationConfig {
    path: string                 # URI prefix, e.g. /cgi-bin/
    root: string                 # optional, overrides server root
    cgi_root: string             # optional
    cgi_extension: string        # optional
    cgi_executable: string       # optional
    index_files: list of string  # optional
};
