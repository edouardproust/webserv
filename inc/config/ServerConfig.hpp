class ServerConfig {
    root: string
    server_name: string
    listen_port: int
    locations: list of LocationConfig

    method parse(block_lines):
        for each line:
            if line starts with "location":
                parse LocationConfig
            else:
                parse server-level directives (root, server_name, listen)

};
