#include "network/Network.hpp"
#include "config/Config.hpp"

int main(int argc, char** argv)
{
	if (argc < 1) {
		Log::prod("error", "Error launching " + Const::SERVER_SOFTWARE + ".");
		return 1;
	} else if (argc > 2) {
		Log::prod("error", "Usage: " + utils::str(argv[0]) + " [config_file]");
		return 1;
	}

	sig::setup();
	try {
		Config config(argc == 1 ? Const::DEFAULT_CONFIG_FILE_PATH : argv[1]);
		Log::dev("debug", "Config:\n" + utils::str(config));
		Network network(config);
		network.startServers();
	} catch (const std::exception& e) {
		Log::prod("error", e.what());
		return 1;
	}

	return 0;
}
