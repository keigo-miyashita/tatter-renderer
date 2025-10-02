#include <sqrap.hpp>

#include "App.hpp"

int main()
{
	App app;

	try {
		if (!app.Init()) {
			throw std::runtime_error("Failed to initialize application.");
		}
		app.Run();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}