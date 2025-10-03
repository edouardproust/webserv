class Config {
    std::vector<ServerConfig>   servers;

    method parse(file_path):
        read file line by line
        for each "server { ... }" block:
            create ServerConfig instance
            parse server block
            append to servers
};
