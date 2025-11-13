#include "network/Network.hpp"
#include "config/Config.hpp"

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}

	sig::setup();
	try {
		Config config(argv[1]);
		Log::dev("debug", "Config:\n" + utils::str(config));
		Network network(config);
		network.startServers();
	} catch (const std::exception& e) {
		Log::prod("error", e.what());
		return 1;
	}

	return 0;
}
